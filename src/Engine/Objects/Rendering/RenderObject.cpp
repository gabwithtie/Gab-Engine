#include "RenderObject.h"
#include <glm/gtx/matrix_decompose.hpp>

#include "Editor/gbe_editor.h"
#include "Graphics/RenderPipeline.h"
#include "Asset/gbe_asset.h"

#include <string>

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
	to_update = RenderPipeline::Get_Instance()->RegisterInstance(this, mDrawCall, this->World().GetMatrix());
}

void gbe::RenderObject::SetUserCreated()
{
	this->user_created = true;
	this->PushEditorFlag(Object::SERIALIZABLE);

	if (Engine::Get_state() == Engine::EngineState::Edit) {
		auto ro = new RigidObject(true);
		ro->SetParent(this);
		ro->PushEditorFlag(Object::SELECT_PARENT_INSTEAD);
		ro->PushEditorFlag(Object::EXCLUDE_FROM_OBJECT_TREE);

		auto col = new MeshCollider(this->mDrawCall->get_mesh());
		col->SetParent(ro);
	}
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
		RenderPipeline::Get_Instance()->UnRegisterInstanceAll(this);
}

void gbe::RenderObject::InvokeEarlyUpdate()
{
	if (to_update != nullptr)
		*to_update = this->World().GetMatrix();
}

void gbe::RenderObject::On_Change_enabled(bool _to) {
	Object::On_Change_enabled(_to);

	RenderPipeline::Get_Instance()->SetEnableInstance(this, _to);
}

gbe::SerializedObject gbe::RenderObject::Serialize() {
	auto data = gbe::Object::Serialize();

	data.serialized_variables.insert_or_assign("usercreated", std::to_string(this->user_created));

	data.serialized_variables.insert_or_assign("primitive", PrimitiveTypeStr(this->ptype));
	data.serialized_variables.insert_or_assign("mesh", this->mDrawCall->get_mesh()->Get_assetId());
	data.serialized_variables.insert_or_assign("mat", this->mDrawCall->get_material()->Get_assetId());

	return data;
}

gbe::RenderObject::RenderObject(SerializedObject* data) : Object(data)
{
	auto _ptype = data->serialized_variables["primitive"];

	if (_ptype == gbe::RenderObject::PrimitiveTypeStr(gbe::RenderObject::PrimitiveType::NONE)) {
		auto input_mesh = MeshLoader::GetAssetByPath(data->serialized_variables["mesh"]);
		auto input_mat = MaterialLoader::GetAssetByPath(data->serialized_variables["mat"]);
		auto drawcall = RenderPipeline::RegisterDrawCall(input_mesh, input_mat);
		
		this->mDrawCall = drawcall;
		to_update = RenderPipeline::Get_Instance()->RegisterInstance(this, mDrawCall, this->World().GetMatrix());
	}
	else {
		auto curptype = gbe::RenderObject::PrimitiveType::NONE;
		for (const auto& pair : gbe::RenderObject::PrimitiveTypeStrs) {
			if (pair.second == _ptype) {
				curptype = pair.first;
				break;
			}
		}

		this->mDrawCall = primitive_drawcalls[curptype];
		to_update = RenderPipeline::Get_Instance()->RegisterInstance(this, mDrawCall, this->World().GetMatrix());
		this->ptype = curptype;
	}

	auto usercreated = data->serialized_variables["usercreated"];

	if (usercreated.size() > 0 && std::stoi(usercreated))
	{
		this->SetUserCreated();
	}
}

std::vector<std::vector<gbe::Vector3>> gbe::RenderObject::GetWorldSpaceVertexes()
{
	auto verts = std::vector<std::vector<Vector3>>();
	auto& src_verts = this->Get_DrawCall()->get_mesh()->Get_load_data().vertices;

	for (const auto& face : this->Get_DrawCall()->get_mesh()->Get_load_data().faces)
	{
		auto newface = std::vector<Vector3>();

		for (const auto& vertindex : face)
		{
			auto pos = Vector4(src_verts[vertindex].pos, 1);
			auto world_pos = Vector3(this->World().GetMatrix() * pos);

			newface.push_back(world_pos);
		}

		verts.push_back(newface);
	}

	return verts;
}