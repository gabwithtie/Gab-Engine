#include "RenderObject.h"
#include <glm/gtx/matrix_decompose.hpp>

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

gbe::RenderObject::RenderObject(DrawCall* mDrawCall, bool editor) : editor(editor)
{
	this->mDrawCall = mDrawCall;
	to_update = this->mDrawCall->RegisterCall(this, this->GetWorldMatrix());
}

gbe::RenderObject::RenderObject(PrimitiveType _ptype, bool editor) : editor(editor)
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
	*to_update = this->GetWorldMatrix();
}

gbe::SerializedObject gbe::RenderObject::Serialize() {
	auto data = gbe::Object::Serialize();

	data.serialized_variables.insert_or_assign("primitive", PrimitiveTypeStr(this->ptype));
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
		return new RenderObject(curptype, this->editor);
	}
}
