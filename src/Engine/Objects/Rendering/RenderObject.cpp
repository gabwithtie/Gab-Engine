#include "RenderObject.h"
#include <glm/gtx/matrix_decompose.hpp>

#include "Editor/gbe_editor.h"
#include "Graphics/RenderPipeline.h"

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

gbe::RenderObject::RenderObject(DrawCall* mDrawCall)
{
	this->mDrawCall = mDrawCall;
	to_update = this->mDrawCall->RegisterCall(this, this->GetWorldMatrix());

	auto texture_field = new gbe::editor::InspectorAsset<TextureLoader, asset::Texture>();
	texture_field->name = "Texture";
	texture_field->choice = &this->_tex;

	auto mesh_field = new gbe::editor::InspectorAsset<MeshLoader, asset::Mesh>();
	mesh_field->name = "Mesh";
	mesh_field->choice = &this->_mesh;

	auto mat_field = new gbe::editor::InspectorAsset<MaterialLoader, asset::Material>();
	mat_field->name = "Material";
	mat_field->choice = &this->_mat;

	auto refresh_field = new gbe::editor::InspectorButton();
	refresh_field->name = "Refresh Drawcall";
	refresh_field->onpress = [this]() {
		auto old_drawcall = this->mDrawCall;
		old_drawcall->UnRegisterCall(this);

		this->_mat->setOverride("colortex", this->_tex);

		this->mDrawCall = RenderPipeline::Get_Instance()->RegisterDrawCall(this->_mesh, this->_mat);
		this->to_update = this->mDrawCall->RegisterCall(this, this->GetWorldMatrix());
		};

	this->inspectorData->fields.push_back(texture_field);
	this->inspectorData->fields.push_back(mesh_field);
	this->inspectorData->fields.push_back(refresh_field);
}

gbe::RenderObject::RenderObject(PrimitiveType _ptype)
{
	this->mDrawCall = primitive_drawcalls[_ptype];
	to_update = this->mDrawCall->RegisterCall(this, this->GetWorldMatrix());
	this->ptype = _ptype;
}

gbe::RenderObject::~RenderObject()
{
	this->mDrawCall->UnRegisterCall(this);
}

void gbe::RenderObject::InvokeEarlyUpdate()
{
	if (to_update == nullptr)
		return;

	*to_update = this->GetWorldMatrix();
}

void gbe::RenderObject::On_Change_enabled(bool _to) {
	Object::On_Change_enabled(_to);

	if (_to) {
		to_update = this->mDrawCall->RegisterCall(this, this->GetWorldMatrix());
	}
	else {
		this->mDrawCall->UnRegisterCall(this);
		to_update = nullptr;
	}
}

gbe::SerializedObject gbe::RenderObject::Serialize() {
	auto data = gbe::Object::Serialize();

	data.serialized_variables.insert_or_assign("primitive", PrimitiveTypeStr(this->ptype));

	return data;
}

gbe::Object* gbe::RenderObject::Create(gbe::SerializedObject data) {
	auto _ptype = data.serialized_variables["primitive"];

	if (_ptype == PrimitiveTypeStr(PrimitiveType::NONE)) {
		return nullptr;
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
