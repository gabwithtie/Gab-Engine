#include "AnitoBuilderWrapper.h"

#include "Engine/gbe_engine.h"
#include "Editor/gbe_editor.h"
#include "Ext/AnitoBuilder/AnitoBuilder.h"

void gbe::ext::AnitoBuilder::AnimoBuilderObject::Regenerate()
{
	if (this->generation_root != nullptr) {
		this->generation_root->SetParent(this->parent);
		this->generation_root->Destroy();
		this->generation_root = nullptr;
	}

	this->generation_root = new GenericObject([](GenericObject* self, float dt) {});

	for (size_t i = 0; i < this->GetChildCount() - 1; i++)
	{
		auto start = this->GetChildAt(i);
		auto end = this->GetChildAt(i + 1);

		this->parentparams.from = start->World().position.Get();
		this->parentparams.to = end->World().position.Get();

		auto builder_result = ext::AnitoBuilder::AnitoBuilder::Generate(this->parentparams);

		//READ THE XML RESULT AND USE EXTERNALLY-LOADED MESHES
		for (auto& objdata : builder_result.meshes)
		{
			auto it = this->drawcall_dictionary.find(objdata.type);

			if (it != this->drawcall_dictionary.end()) {
				this->CreateMesh(this->drawcall_dictionary[objdata.type], objdata.position, objdata.scale, objdata.rotation);
			}
		}
	}

	this->generation_root->SetParent(this);
}

void gbe::ext::AnitoBuilder::AnimoBuilderObject::CreateMesh(gfx::DrawCall* drawcall, Vector3 pos, Vector3 scale, Quaternion rotation)
{
	RigidObject* parent = new RigidObject(true);
	parent->SetParent(this->generation_root);
	parent->World().position.Set(pos);
	parent->Local().rotation.Set(rotation);
	parent->Local().scale.Set(scale);
	RenderObject* platform_renderer = new RenderObject(drawcall);
	platform_renderer->SetParent(parent);
}

gbe::ext::AnitoBuilder::AnimoBuilderObject::AnimoBuilderObject()
{
	//DRAWCALL CACHING OF DEFAULT BOX
	auto cube_mesh = asset::Mesh::GetAssetById("cube");
	auto cube_mat = asset::Material::GetAssetById("grid");
	this->cube_drawcall = RenderPipeline::Get_Instance()->RegisterDrawCall(cube_mesh, cube_mat);

	//DRAWCALL CACHING OF EVERYTHING ELSE
	//MESH AND DRAWCALLS FOR ANIMOBUILDER
	auto roof_mat = asset::Material::GetAssetById("unlit");
	auto roof_tex = asset::Texture::GetAssetById("roof");
	roof_mat->setOverride("colortex", roof_tex);
	auto roof_m = asset::Mesh::GetAssetById("roof");
	auto roof_dc = RenderPipeline::Get_Instance()->RegisterDrawCall(roof_m, roof_mat);
	drawcall_dictionary.insert_or_assign("roof", roof_dc);

	auto window_mat = asset::Material::GetAssetById("unlit");
	//auto window_tex = asset::Texture::GetAssetById("window");
	//window_mat->setOverride("colortex", roof_tex);
	auto window_m = asset::Mesh::GetAssetById("window");
	auto window_dc = RenderPipeline::Get_Instance()->RegisterDrawCall(window_m, window_mat);
	//drawcall_dictionary.insert_or_assign("window", window_dc);

	auto pillar_mat = asset::Material::GetAssetById("unlit");
	auto pillar_tex = asset::Texture::GetAssetById("pillar");
	pillar_mat->setOverride("colortex", pillar_tex);
	auto pillar_m = asset::Mesh::GetAssetById("pillar");
	auto pillar_dc = RenderPipeline::Get_Instance()->RegisterDrawCall(pillar_m, pillar_mat);
	drawcall_dictionary.insert_or_assign("pillar", pillar_dc);

	auto wall_mat = asset::Material::GetAssetById("unlit");
	auto wall_tex = asset::Texture::GetAssetById("wall");
	wall_mat->setOverride("colortex", wall_tex);
	auto wall_m = asset::Mesh::GetAssetById("wall");
	auto wall_dc = RenderPipeline::Get_Instance()->RegisterDrawCall(wall_m, wall_mat);
	drawcall_dictionary.insert_or_assign("wall", wall_dc);

	//INSPECTOR BUTTON
	auto spawn_button = new editor::InspectorButton();
	spawn_button->name = "Regenerate";
	spawn_button->onpress = [this]() {
		this->Regenerate();
		};

	this->inspectorData->fields.push_back(spawn_button);
}

void gbe::ext::AnitoBuilder::AnimoBuilderObject::AddPillar(Vector3 position)
{
	RigidObject* test = new RigidObject(true);
	test->SetParent(this);
	test->Local().position.Set(position);
	test->Local().rotation.Set(Quaternion::Euler(Vector3(0)));
	test->Local().scale.Set(Vector3(0.5f, 2, 0.5f));
	BoxCollider* platform_collider = new BoxCollider();
	platform_collider->SetParent(test);
	platform_collider->Local().position.Set(Vector3(0, 0, 0));
	RenderObject* platform_renderer = new RenderObject(this->cube_drawcall);
	platform_renderer->SetParent(test);
}
