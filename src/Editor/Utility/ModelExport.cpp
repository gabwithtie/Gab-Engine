#include "ModelExport.h"
#include "Asset/AssetTypes/Mesh.h" // Assuming this gives access to asset::Mesh::Get_LoadData()

#include <assimp/scene.h>
#include <assimp/matrix4x4.h>
#include <assimp/vector3.h>

#include <sstream>
#include <ostream>
#include <filesystem>
#include <iostream>
#include <string> // For std::to_string

#include <omp.h>
#include <mutex>
#include <atomic>
#include <iomanip>

namespace gbe {
    namespace editor {

        // --- Assimp Helper Conversions ---

        // Helper function to map your Vector3 to Assimp's aiVector3D
        aiVector3D ToAiVector3D(const Vector3& v) {
            return aiVector3D(v.x, v.y, v.z);
        }

        // Helper function to map your Vector2 to Assimp's aiVector2D
        aiVector2D ToAiVector2D(const Vector2& v) {
            return aiVector2D(v.x, v.y);
        }

        // Helper function to map your Matrix4 to Assimp's aiMatrix4x4.
        // ASSUMPTION: Matrix4 has a public array 'm' of 16 floats, column-major.
        aiMatrix4x4 ToAiMatrix4x4(const Matrix4& m) {
            // Assimp matrix construction expects elements in a specific order:
            // a1, a2, a3, a4 (col 1), b1, b2, b3, b4 (col 2), etc.
            return aiMatrix4x4(
                // Row 1 (X)
                m[0][0], m[1][0], m[2][0], m[3][0],
                // Row 2 (Y)
                m[0][1], m[1][1], m[2][1], m[3][1],
                // Row 3 (Z)
                m[0][2], m[1][2], m[2][2], m[3][2],
                // Row 4 (W)
                m[0][3], m[1][3], m[2][3], m[3][3]
            );
        }

        // --- Core ModelExport Implementation ---

        ModelExport::ModelExport(std::vector<gbe::Object*> selected)
        {
            this->selected = selected;
        }

        void ModelExport::Export(std::filesystem::path path)
        {
            // --- Stage 1: Discovery ---
            std::cout << "[1/3] Gathering Renderers..." << std::endl;
            std::vector<export_subobject> export_subobjects;
            std::vector<RenderObject*> renderers;

            for (const auto& root_object : this->selected) {
                root_object->CallRecursively([&](Object* obj) {
                    RenderObject* target = dynamic_cast<RenderObject*>(obj);
                    if (target && target->Get_enabled()) {
                        if (std::find(renderers.begin(), renderers.end(), target) == renderers.end()) {
                            renderers.push_back(target);
                        }
                    }
                    });
            }

            for (const auto& renderer : renderers) {
                export_subobject new_subobject;
                new_subobject.source_mesh = &renderer->Get_DrawCall()->get_mesh()->Get_load_data();
                new_subobject.transform = renderer->World().GetMatrix();
                export_subobjects.push_back(new_subobject);
            }

            if (export_subobjects.empty()) return;

            // --- Stage 2: Parallel Processing ---
            std::cout << "[2/3] Processing " << export_subobjects.size() << " meshes on "
                << omp_get_max_threads() << " threads..." << std::endl;

            Assimp::Exporter exporter;
            aiScene scene;
            scene.mNumMeshes = static_cast<unsigned int>(export_subobjects.size());
            scene.mMeshes = new aiMesh * [scene.mNumMeshes]();
            scene.mNumMaterials = scene.mNumMeshes;
            scene.mMaterials = new aiMaterial * [scene.mNumMaterials]();
            scene.mRootNode = new aiNode("SceneRoot");

            std::mutex scene_mutex;
            std::atomic<int> completed_meshes{ 0 };
            int total_meshes = static_cast<int>(export_subobjects.size());

#pragma omp parallel for schedule(dynamic)
            for (int i = 0; i < total_meshes; ++i) {
                const auto& subobject = export_subobjects[i];
                const auto& mesh_data = subobject.source_mesh;

                if (mesh_data->vertices.empty()) {
                    completed_meshes++;
                    continue;
                }

                aiMesh* local_mesh = new aiMesh();
                local_mesh->mName = "Mesh_" + std::to_string(i);
                local_mesh->mPrimitiveTypes = aiPrimitiveType_TRIANGLE;

                size_t v_count = mesh_data->vertices.size();
                local_mesh->mNumVertices = static_cast<unsigned int>(v_count);
                local_mesh->mVertices = new aiVector3D[v_count];
                local_mesh->mNormals = new aiVector3D[v_count];
                local_mesh->mTextureCoords[0] = new aiVector3D[v_count];
                local_mesh->mNumUVComponents[0] = 2;
                local_mesh->mColors[0] = new aiColor4D[v_count];

                // Silent high-speed vertex copy
                for (size_t v = 0; v < v_count; ++v) {
                    const auto& vtx = mesh_data->vertices[v];
                    local_mesh->mVertices[v] = ToAiVector3D(vtx.pos);
                    local_mesh->mNormals[v] = ToAiVector3D(vtx.normal);
                    local_mesh->mTextureCoords[0][v] = aiVector3D(vtx.texCoord.x, vtx.texCoord.y, 0.0f);
                    local_mesh->mColors[0][v] = aiColor4D(vtx.color.x, vtx.color.y, vtx.color.z, 1.0f);
                }

                size_t f_count = mesh_data->indices.size() / 3;
                local_mesh->mNumFaces = static_cast<unsigned int>(f_count);
                local_mesh->mFaces = new aiFace[f_count];
                for (size_t f = 0; f < f_count; ++f) {
                    aiFace& face = local_mesh->mFaces[f];
                    face.mNumIndices = 3;
                    face.mIndices = new unsigned int[3] { mesh_data->indices[f * 3], mesh_data->indices[f * 3 + 1], mesh_data->indices[f * 3 + 2] };
                }

                {
                    std::lock_guard<std::mutex> lock(scene_mutex);
                    scene.mMeshes[i] = local_mesh;
                    aiNode* node = new aiNode(local_mesh->mName.C_Str());
                    node->mTransformation = ToAiMatrix4x4(subobject.transform);
                    node->mNumMeshes = 1;
                    node->mMeshes = new unsigned int[1] { (unsigned int)i };
                    scene.mRootNode->addChildren(1, &node);

                    aiMaterial* material = new aiMaterial();
                    aiString mat_name("Material_" + std::to_string(i));
                    material->AddProperty(&mat_name, AI_MATKEY_NAME);
                    scene.mMaterials[i] = material;
                }

                // Smart Progress Logging: Only one thread prints, and only occasionally
                int current_count = ++completed_meshes;
                if (current_count % 5 == 0 || current_count == total_meshes) {
#pragma omp critical(logging)
                    {
                        float pct = (float)current_count / total_meshes * 100.0f;
                        std::cout << "\r> Progress: " << std::fixed << std::setprecision(1)
                            << pct << "% (" << current_count << "/" << total_meshes << " meshes)" << std::flush;
                    }
                }
            }
            std::cout << std::endl;

            // --- Stage 3: File Writing ---
            std::cout << "[3/3] Writing file to disk..." << std::endl;
            std::string ext = path.extension().string();
            if (ext.length() > 1) ext = ext.substr(1);

            if (exporter.Export(&scene, ext.c_str(), path.string().c_str()) == AI_SUCCESS) {
                std::cout << ">> Export Complete: " << path.filename() << std::endl;
            }
            else {
                std::cerr << ">> Export Failed: " << exporter.GetErrorString() << std::endl;
            }
        }
    } // namespace editor
} // namespace gbe