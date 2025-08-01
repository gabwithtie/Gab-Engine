#include "RenderObject.h"
#include <glm/gtx/matrix_decompose.hpp>

using namespace gbe::gfx;

std::unordered_map<gbe::RenderObject::PrimitiveType, gbe::gfx::DrawCall*> gbe::RenderObject::primitive_drawcalls;

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
