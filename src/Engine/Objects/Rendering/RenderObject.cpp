#include "RenderObject.h"
#include <glm/gtx/matrix_decompose.hpp>

#include "Editor/gbe_editor.h"
#include "Graphics/RenderPipeline.h"
#include "Asset/gbe_asset.h"

using namespace gbe::gfx;

std::unordered_map<gbe::RenderObject::PrimitiveType, gbe::gfx::DrawCall*> gbe::RenderObject::primitive_drawcalls;

const std::unordered_map<gbe::RenderObject::PrimitiveType, std::string> gbe::RenderObject::PrimitiveTypeStrs =
{
	{ PrimitiveType::NONE, "None" },
	{ PrimitiveType::cube, "Cube" },
	{ PrimitiveType::sphere, "Sphere" },
	{ PrimitiveType::capsule, "Capsule" },
	{ PrimitiveType::plane, "Plane" }
};

void gbe::RenderObject::SetShadowCaster()
{
	auto shadow_drawcall = RenderPipeline::RegisterDrawCall(this->mDrawCall->get_mesh(), asset::Material::GetAssetById("shadow"), -1);
	this->shadow_renderer = new RenderObject(shadow_drawcall);
	this->shadow_renderer->SetParent(this);
}

gbe::RenderObject::RenderObject(DrawCall* mDrawCall)
{
	this->mDrawCall = mDrawCall;
	to_update = RenderPipeline::Get_Instance()->RegisterInstance(this, mDrawCall, this->World().GetMatrix());

	auto texture_field = new gbe::editor::InspectorAsset<TextureLoader, asset::Texture>();
	texture_field->name = "Texture";
	texture_field->choice = (asset::internal::BaseAsset_base**)&this->input_tex;

	auto mesh_field = new gbe::editor::InspectorAsset<MeshLoader, asset::Mesh>();
	mesh_field->name = "Mesh";
	mesh_field->choice = (asset::internal::BaseAsset_base**)&this->input_mesh;

	auto mat_field = new gbe::editor::InspectorAsset<MaterialLoader, asset::Material>();
	mat_field->name = "Material";
	mat_field->choice = (asset::internal::BaseAsset_base**)&this->input_mat;

	auto refresh_field = new gbe::editor::InspectorButton();
	refresh_field->name = "Refresh Drawcall";
	refresh_field->onpress = [=]() {
		auto old_drawcall = this->mDrawCall;
		if(old_drawcall != nullptr)
			RenderPipeline::Get_Instance()->UnRegisterCall(this);

		this->input_mat->setOverride("colortex", this->input_tex);

		this->mDrawCall = RenderPipeline::Get_Instance()->RegisterDrawCall(this->input_mesh, this->input_mat);
		this->to_update = RenderPipeline::Get_Instance()->RegisterInstance(this, mDrawCall, this->World().GetMatrix());
		};

	this->inspectorData->fields.push_back(texture_field);
	this->inspectorData->fields.push_back(mesh_field);
	this->inspectorData->fields.push_back(mat_field);
	this->inspectorData->fields.push_back(refresh_field);
}

gbe::RenderObject::RenderObject(PrimitiveType _ptype)
{
	this->mDrawCall = primitive_drawcalls[_ptype];
	to_update = RenderPipeline::Get_Instance()->RegisterInstance(this, mDrawCall, this->World().GetMatrix());
	this->ptype = _ptype;
}

gbe::RenderObject::~RenderObject()
{
	if (to_update != nullptr)
		RenderPipeline::Get_Instance()->UnRegisterCall(this);
}

void gbe::RenderObject::InvokeEarlyUpdate()
{
	if (to_update != nullptr)
		*to_update = this->World().GetMatrix();
}

void gbe::RenderObject::On_Change_enabled(bool _to) {
	Object::On_Change_enabled(_to);

	if (to_update == nullptr && _to) {
		to_update = RenderPipeline::Get_Instance()->RegisterInstance(this, mDrawCall, this->World().GetMatrix());
	}
	else if(to_update != nullptr) {
		RenderPipeline::Get_Instance()->UnRegisterCall(this);
		to_update = nullptr;
	}
}

gbe::SerializedObject gbe::RenderObject::Serialize() {
	auto data = gbe::Object::Serialize();

	data.serialized_variables.insert_or_assign("primitive", PrimitiveTypeStr(this->ptype));

	std::string mesh_str;
	if (this->input_mesh != nullptr)
		mesh_str = this->input_mesh->Get_asset_filepath().string();
	else
		mesh_str = "";
	data.serialized_variables.insert_or_assign("mesh", mesh_str);

	std::string tex_str;
	if (this->input_tex != nullptr)
		tex_str = this->input_tex->Get_asset_filepath().string();
	else
		tex_str = "";
	data.serialized_variables.insert_or_assign("tex", tex_str);

	std::string mat_str;
	if (this->input_tex != nullptr)
		mat_str = this->input_mat->Get_asset_filepath().string();
	else
		mat_str = "";
	data.serialized_variables.insert_or_assign("mat", mat_str);

	return data;
}

gbe::Object* gbe::RenderObject::Create(gbe::SerializedObject data) {
	auto _ptype = data.serialized_variables["primitive"];

	if (_ptype == PrimitiveTypeStr(PrimitiveType::NONE)) {
		auto newobj = new RenderObject(nullptr);

		newobj->input_mesh = MeshLoader::GetAssetByPath(data.serialized_variables["mesh"]);
		newobj->input_mat = MaterialLoader::GetAssetByPath(data.serialized_variables["mat"]);
		newobj->input_tex = TextureLoader::GetAssetByPath(data.serialized_variables["tex"]);

		newobj->input_mat->setOverride("colortex", newobj->input_tex);

		newobj->mDrawCall = RenderPipeline::Get_Instance()->RegisterDrawCall(newobj->input_mesh, newobj->input_mat);
		newobj->to_update = RenderPipeline::Get_Instance()->RegisterInstance(newobj, newobj->mDrawCall, newobj->World().GetMatrix());

		return newobj;
	}
	else {
		auto curptype = PrimitiveType::NONE;
		for (const auto& pair : PrimitiveTypeStrs) {
			if (pair.second == _ptype) {
				curptype = pair.first;
				break;
			}
		}
		return new RenderObject(curptype);
	}
}
