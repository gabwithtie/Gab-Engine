#include "LightObject.h"

using namespace gbe;

void LightObject::Set_Color(Vector3 color) {
	this->GetData()->color = color;
	this->changed = true;
}

void gbe::LightObject::OnLocalTransformationChange(TransformChangeType type)
{
	Object::OnLocalTransformationChange(type);

	this->changed = true;
}

void gbe::LightObject::OnExternalTransformationChange(TransformChangeType type, Matrix4 parentmat)
{
	Object::OnExternalTransformationChange(type, parentmat);

	this->changed = true;
}
