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
	data.serialized_variables.insert_or_assign("mesh", this->mDrawCall->get_mesh()->Get_assetId());
	data.serialized_variables.insert_or_assign("mat", this->mDrawCall->get_material()->Get_assetId());
	data.serialized_variables.insert_or_assign("order", std::to_string(this->mDrawCall->order));

	return data;
}
namespace gbe
{
	template<>
	std::function<gbe::RenderObject* (gbe::SerializedObject)> gbe::RenderObject::create_func<gbe::RenderObject> = [](gbe::SerializedObject data) {
		auto _ptype = data.serialized_variables["primitive"];

		if (_ptype == gbe::RenderObject::PrimitiveTypeStr(gbe::RenderObject::PrimitiveType::NONE)) {
			auto input_mesh = MeshLoader::GetAssetByPath(data.serialized_variables["mesh"]);
			auto input_mat = MaterialLoader::GetAssetByPath(data.serialized_variables["mat"]);
			auto input_order = std::stoi(data.serialized_variables["order"]);
			auto drawcall = RenderPipeline::RegisterDrawCall(input_mesh, input_mat, input_order);
			auto newobj = new RenderObject(nullptr);
			return newobj;
		}
		else {
			auto curptype = gbe::RenderObject::PrimitiveType::NONE;
			for (const auto& pair : gbe::RenderObject::PrimitiveTypeStrs) {
				if (pair.second == _ptype) {
					curptype = pair.first;
					break;
				}
			}
			return new RenderObject(curptype);
		}
		};
}