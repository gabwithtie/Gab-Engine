#include "AnimoBuilderWrapper.h"

#include "Editor/gbe_editor.h"
#include "Ext/AnimoBuilder/AnimoBuilder.h"

void gbe::ext::AnimoBuilder::AnimoBuilderObject::Regenerate()
{
	if (this->generation_root != nullptr) {
		this->generation_root->Destroy();
		this->generation_root = nullptr;
	}

	this->generation_root = new GenericObject([](GenericObject* self, float dt) {});

	ext::AnimoBuilder::GenerationParams params{};
	auto builder_result = ext::AnimoBuilder::AnimoBuilder::Generate(params);

	//READ THE XML RESULT AND USE EXTERNALLY-LOADED MESHES
	for (auto& objdata : builder_result.meshes)
	{
		this->CreateMesh(this->drawcall_dictionary[objdata.type], objdata.position, objdata.scale, Quaternion::Euler(Vector3(0, 0, 0)));
	}
}

void gbe::ext::AnimoBuilder::AnimoBuilderObject::CreateMesh(gfx::DrawCall* drawcall, Vector3 pos, Vector3 scale, Quaternion rotation)
{
	RigidObject* parent = new RigidObject(true);
	parent->SetParent(this->generation_root);
	parent->Local().position.Set(pos);
	parent->Local().rotation.Set(rotation);
	parent->Local().scale.Set(scale);
	MeshCollider* meshcollider = new MeshCollider(drawcall->get_mesh());
	meshcollider->SetParent(parent);
	meshcollider->Local().position.Set(Vector3(0, 0, 0));
	RenderObject* platform_renderer = new RenderObject(drawcall);
	platform_renderer->SetParent(parent);
}

gbe::ext::AnimoBuilder::AnimoBuilderObject::AnimoBuilderObject()
{
	//DRAWCALL CACHING OF DEFAULT BOX
	auto cube_mesh = new asset::Mesh("DefaultAssets/3D/default/cube.obj.gbe");
	auto cube_mat = new asset::Material("DefaultAssets/Materials/grid.mat.gbe");
	this->cube_drawcall = RenderPipeline::Get_Instance()->RegisterDrawCall(cube_mesh, cube_mat);

	//DRAWCALL CACHING OF EVERYTHING ELSE
	//MESH AND DRAWCALLS FOR ANIMOBUILDER
	auto roof_mat = new asset::Material("DefaultAssets/Materials/unlit.mat.gbe");
	auto roof_tex = new asset::Texture("DefaultAssets/Tex/Maps/Model/roof.img.gbe");
	roof_mat->setOverride("colortex", roof_tex);
	auto roof_m = new asset::Mesh("DefaultAssets/3D/builder/roof.obj.gbe");
	auto roof_dc = RenderPipeline::Get_Instance()->RegisterDrawCall(roof_m, roof_mat);
	drawcall_dictionary.insert_or_assign("roof", roof_dc);

	auto window_mat = new asset::Material("DefaultAssets/Materials/unlit.mat.gbe");
	//auto window_tex = new asset::Texture("DefaultAssets/Tex/Maps/Model/window.img.gbe");
	//window_mat->setOverride("colortex", roof_tex);
	auto window_m = new asset::Mesh("DefaultAssets/3D/builder/window.obj.gbe");
	auto window_dc = RenderPipeline::Get_Instance()->RegisterDrawCall(window_m, window_mat);
	drawcall_dictionary.insert_or_assign("window", window_dc);

	auto pillar_mat = new asset::Material("DefaultAssets/Materials/unlit.mat.gbe");
	auto pillar_tex = new asset::Texture("DefaultAssets/Tex/Maps/Model/pillar.img.gbe");
	pillar_mat->setOverride("colortex", pillar_tex);
	auto pillar_m = new asset::Mesh("DefaultAssets/3D/builder/pillar.obj.gbe");
	auto pillar_dc = RenderPipeline::Get_Instance()->RegisterDrawCall(pillar_m, pillar_mat);
	drawcall_dictionary.insert_or_assign("pillar", pillar_dc);

	auto wall_mat = new asset::Material("DefaultAssets/Materials/unlit.mat.gbe");
	auto wall_tex = new asset::Texture("DefaultAssets/Tex/Maps/Model/wall.img.gbe");
	wall_mat->setOverride("colortex", wall_tex);
	auto wall_m = new asset::Mesh("DefaultAssets/3D/builder/wall.obj.gbe");
	auto wall_dc = RenderPipeline::Get_Instance()->RegisterDrawCall(wall_m, wall_mat);
	drawcall_dictionary.insert_or_assign("wall", wall_dc);

	//INSPECTOR BUTTON
	auto spawn_button = new editor::InspectorButton();
	spawn_button->name = "Spawn Node";
	spawn_button->onpress = [this]() {
		this->Regenerate();
		};

	this->inspectorData->fields.push_back(spawn_button);
}

void gbe::ext::AnimoBuilder::AnimoBuilderObject::AddPillar(Vector3 position)
{
	RigidObject* test = new RigidObject();
	test->SetParent(this);
	test->Local().position.Set(position);
	test->Local().rotation.Set(Quaternion::Euler(Vector3(0)));
	test->Local().scale.Set(Vector3(1));
	BoxCollider* platform_collider = new BoxCollider();
	platform_collider->SetParent(test);
	platform_collider->Local().position.Set(Vector3(0, 0, 0));
	RenderObject* platform_renderer = new RenderObject(cube_drawcall);
	platform_renderer->SetParent(test);
}
