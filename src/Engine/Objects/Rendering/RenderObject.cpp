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

gbe::RenderObject::RenderObject(DrawCall* mDrawCall, int order)
{
	this->order = order;

	if (mDrawCall != nullptr) {
		this->mDrawCall = mDrawCall;
		to_update = RenderPipeline::Get_Instance()->RegisterCall(this, mDrawCall, this->World().GetMatrix(), order);
	}

	auto texture_field = new gbe::editor::InspectorAsset<TextureLoader, asset::Texture>();
	texture_field->name = "Texture";
	texture_field->choice = (asset::internal::BaseAsset_base**)&this->_tex;

	auto mesh_field = new gbe::editor::InspectorAsset<MeshLoader, asset::Mesh>();
	mesh_field->name = "Mesh";
	mesh_field->choice = (asset::internal::BaseAsset_base**)&this->_mesh;

	auto mat_field = new gbe::editor::InspectorAsset<MaterialLoader, asset::Material>();
	mat_field->name = "Material";
	mat_field->choice = (asset::internal::BaseAsset_base**)&this->_mat;

	auto refresh_field = new gbe::editor::InspectorButton();
	refresh_field->name = "Refresh Drawcall";
	refresh_field->onpress = [=]() {
		auto old_drawcall = this->mDrawCall;
		if(old_drawcall != nullptr)
			RenderPipeline::Get_Instance()->UnRegisterCall(this);

		this->_mat->setOverride("colortex", this->_tex);

		this->mDrawCall = RenderPipeline::Get_Instance()->RegisterDrawCall(this->_mesh, this->_mat);
		this->to_update = RenderPipeline::Get_Instance()->RegisterCall(this, mDrawCall, this->World().GetMatrix(), order);
		};

	this->inspectorData->fields.push_back(texture_field);
	this->inspectorData->fields.push_back(mesh_field);
	this->inspectorData->fields.push_back(mat_field);
	this->inspectorData->fields.push_back(refresh_field);
}

gbe::RenderObject::RenderObject(PrimitiveType _ptype, int order)
{
	this->order = order;

	this->mDrawCall = primitive_drawcalls[_ptype];
	to_update = RenderPipeline::Get_Instance()->RegisterCall(this, mDrawCall, this->World().GetMatrix(), order);
	this->ptype = _ptype;
}

gbe::RenderObject::~RenderObject()
{
	RenderPipeline::Get_Instance()->UnRegisterCall(this);
}

void gbe::RenderObject::InvokeEarlyUpdate()
{
	if (to_update == nullptr)
		return;

	*to_update = this->World().GetMatrix();
}

void gbe::RenderObject::On_Change_enabled(bool _to) {
	Object::On_Change_enabled(_to);

	if (_to) {
		to_update = RenderPipeline::Get_Instance()->RegisterCall(this, mDrawCall, this->World().GetMatrix(), this->order);
	}
	else {
		RenderPipeline::Get_Instance()->UnRegisterCall(this);
		to_update = nullptr;
	}
}

gbe::SerializedObject gbe::RenderObject::Serialize() {
	auto data = gbe::Object::Serialize();

	data.serialized_variables.insert_or_assign("primitive", PrimitiveTypeStr(this->ptype));

	std::string mesh_str;
	if (this->_mesh != nullptr)
		mesh_str = this->_mesh->Get_asset_filepath();
	else
		mesh_str = "";
	data.serialized_variables.insert_or_assign("mesh", mesh_str);

	std::string tex_str;
	if (this->_tex != nullptr)
		tex_str = this->_tex->Get_asset_filepath();
	else
		tex_str = "";
	data.serialized_variables.insert_or_assign("tex", tex_str);

	std::string mat_str;
	if (this->_tex != nullptr)
		mat_str = this->_mat->Get_asset_filepath();
	else
		mat_str = "";
	data.serialized_variables.insert_or_assign("mat", mat_str);

	return data;
}

gbe::Object* gbe::RenderObject::Create(gbe::SerializedObject data) {
	auto _ptype = data.serialized_variables["primitive"];

	if (_ptype == PrimitiveTypeStr(PrimitiveType::NONE)) {
		auto newobj = new RenderObject(nullptr);

		newobj->_mesh = MeshLoader::GetAssetByPath(data.serialized_variables["mesh"]);
		newobj->_mat = MaterialLoader::GetAssetByPath(data.serialized_variables["mat"]);
		newobj->_tex = TextureLoader::GetAssetByPath(data.serialized_variables["tex"]);

		newobj->_mat->setOverride("colortex", newobj->_tex);

		newobj->mDrawCall = RenderPipeline::Get_Instance()->RegisterDrawCall(newobj->_mesh, newobj->_mat);
		newobj->to_update = RenderPipeline::Get_Instance()->RegisterCall(newobj, newobj->mDrawCall, newobj->World().GetMatrix(), newobj->order);

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
