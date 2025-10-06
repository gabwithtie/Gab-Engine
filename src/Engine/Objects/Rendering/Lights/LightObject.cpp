#include "LightObject.h"
#include "Editor/gbe_editor.h"

using namespace gbe;

LightObject::LightObject() {
	auto lightcolor = new gbe::editor::InspectorColor();
	lightcolor->name = "Color";
	lightcolor->r = &this->mLight.color.x;
	lightcolor->g = &this->mLight.color.y;
	lightcolor->b = &this->mLight.color.z;

	this->inspectorData->fields.push_back(lightcolor);

	auto bias_min = new gbe::editor::InspectorFloat();
	bias_min->name = "Bias min";
	bias_min->x = &this->mLight.bias_min;

	this->inspectorData->fields.push_back(bias_min);

	auto bias_mult = new gbe::editor::InspectorFloat();
	bias_mult->name = "Bias mult";
	bias_mult->x = &this->mLight.bias_mult;

	this->inspectorData->fields.push_back(bias_mult);
}

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
