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
            std::vector<export_subobject> export_subobjects;
            std::vector<RenderObject*> renderers;

            // 1. Gather all unique RenderObject components
            for (const auto& root_object : this->selected)
            {
                root_object->CallRecursively([&](Object* obj) {
                    RenderObject* target = dynamic_cast<RenderObject*>(obj);

                    if (target != nullptr && target->Get_enabled() && !target->shadow_caster) {
                        auto it = std::find(renderers.begin(), renderers.end(), target);

                        // Check if the value was found
                        if (it == renderers.end()) {
                            renderers.push_back(target);
                        }
                    }
                    });
            }

            // 2. Map renderers to export sub-objects with their world transforms
            for (const auto& renderer : renderers)
            {
                export_subobject new_subobject;
                asset::Mesh* mesh = renderer->Get_DrawCall()->get_mesh();

                new_subobject.source_mesh = &mesh->Get_load_data();
                new_subobject.transform = renderer->World().GetMatrix(); // Store the world matrix

                export_subobjects.push_back(new_subobject);
            }

            // 3. Assimp Multi-Mesh Export Logic
            if (export_subobjects.empty()) {
                std::cerr << "ModelExport Error: No renderable objects found to export." << std::endl;
                return;
            }

            Assimp::Exporter exporter;
            aiScene scene;

            // Allocate space for all meshes and materials (one of each per sub-object for simplicity)
            scene.mNumMeshes = static_cast<unsigned int>(export_subobjects.size());
            scene.mMeshes = new aiMesh * [scene.mNumMeshes];
            scene.mNumMaterials = scene.mNumMeshes;
            scene.mMaterials = new aiMaterial * [scene.mNumMaterials];

            // Create the root node
            scene.mRootNode = new aiNode("SceneRoot");

            for (size_t i = 0; i < export_subobjects.size(); ++i) {
                const auto& subobject = export_subobjects[i];

                // Fetch the mesh data (assuming asset::Mesh has a Get_LoadData method)
                const auto& mesh_data = subobject.source_mesh;

                if (mesh_data->vertices.empty()) {
                    std::cerr << "Warning: Skipping empty mesh in sub-object " << i << std::endl;
                    continue;
                }

                std::string mesh_name = "Mesh_" + std::to_string(i);

                // --- A. Create and Populate aiMesh (Untransformed - local space) ---
                aiMesh* local_mesh = new aiMesh();
                local_mesh->mName = mesh_name;
                local_mesh->mPrimitiveTypes = aiPrimitiveType_TRIANGLE;

                const size_t num_vertices = mesh_data->vertices.size();
                local_mesh->mNumVertices = static_cast<unsigned int>(num_vertices);

                // Allocate memory
                local_mesh->mVertices = new aiVector3D[num_vertices];
                local_mesh->mNormals = new aiVector3D[num_vertices];
                local_mesh->mTangents = new aiVector3D[num_vertices];
                local_mesh->mTextureCoords[0] = new aiVector3D[num_vertices];
                local_mesh->mNumUVComponents[0] = 2;
                local_mesh->mColors[0] = new aiColor4D[num_vertices];

                // Copy UNTRANSFORMED Vertex Data
                for (size_t v = 0; v < num_vertices; ++v) {
                    const auto& vtx = mesh_data->vertices[v];
                    local_mesh->mVertices[v] = ToAiVector3D(vtx.pos);
                    local_mesh->mNormals[v] = ToAiVector3D(vtx.normal);
                    local_mesh->mTangents[v] = ToAiVector3D(vtx.tangent);
                    local_mesh->mTextureCoords[0][v] = aiVector3D(vtx.texCoord.x, vtx.texCoord.y, 0.0f);
                    local_mesh->mColors[0][v] = aiColor4D(vtx.color.x, vtx.color.y, vtx.color.z, 1.0f);
					std::cout << "Copying vertex (" << std::to_string(v) << "/" << std::to_string(num_vertices) << ")\r";
                }

                // Copy Face/Index Data
                const size_t num_faces = mesh_data->indices.size() / 3;
                local_mesh->mNumFaces = static_cast<unsigned int>(num_faces);
                local_mesh->mFaces = new aiFace[num_faces];
                local_mesh->mMaterialIndex = static_cast<unsigned int>(i); // Link to the new material

                for (size_t f = 0; f < num_faces; ++f) {
                    aiFace& face = local_mesh->mFaces[f];
                    face.mNumIndices = 3;
                    face.mIndices = new unsigned int[3];
                    face.mIndices[0] = mesh_data->indices[f * 3 + 0];
                    face.mIndices[1] = mesh_data->indices[f * 3 + 1];
                    face.mIndices[2] = mesh_data->indices[f * 3 + 2];
                    std::cout << "Copying face (" << std::to_string(f) << "/" << std::to_string(num_faces) << ")\r";
                }

                // --- B. Create aiNode and Link to aiMesh/aiMaterial ---

                scene.mMeshes[i] = local_mesh; // Add mesh to scene array

                aiNode* node = new aiNode(mesh_name);

                // Set the node's transformation matrix (The object's World transform)
                node->mTransformation = ToAiMatrix4x4(subobject.transform);

                // Link the mesh to the node
                node->mNumMeshes = 1;
                node->mMeshes = new unsigned int[1];
                node->mMeshes[0] = static_cast<unsigned int>(i); // Link to the scene's mesh array index

                // Attach the node to the root node
                scene.mRootNode->addChildren(1, &node);

                // --- C. Create aiMaterial (A simple default for each sub-object) ---

                aiMaterial* material = new aiMaterial();
                aiString mat_name("DefaultMaterial_" + std::to_string(i));
                material->AddProperty(&mat_name, AI_MATKEY_NAME);

                scene.mMaterials[i] = material; // Add material to scene array
            }

            // 4. Perform the export
            std::cout << "Performing Mesh Export." << std::endl;


            // Determine the format ID from the file extension
            std::string format_id = path.extension().string();
            if (format_id.empty() || format_id.length() < 2) {
                std::cerr << "ModelExport Error: File path has no valid extension. Cannot determine export format." << std::endl;
                return;
            }
            format_id = format_id.substr(1); // Remove the leading dot

            aiReturn result = exporter.Export(
                &scene,
                format_id.c_str(),
                path.string().c_str()
            );

            // The aiScene destructor handles the cleanup of all created aiMesh, aiNode, and aiMaterial pointers.

            if (result != AI_SUCCESS) {
                std::cerr << "Assimp Export FAILED for format '" << format_id << "': " << exporter.GetErrorString() << std::endl;
            }
            else {
                std::cout << "Assimp Export SUCCESS to: " << path << " using format: " << format_id << std::endl;
            }
        }
    } // namespace editor
} // namespace gbe