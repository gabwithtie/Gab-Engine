#include "LightObject.h"
#include "Editor/gbe_editor.h"

#include <string>

using namespace gbe;

LightObject::LightObject() {
	
}

SerializedObject gbe::LightObject::Serialize()
{
	auto obj = Object::Serialize();

	obj.serialized_variables.insert_or_assign("color_r", std::to_string(mLight.color.x));
	obj.serialized_variables.insert_or_assign("color_g", std::to_string(mLight.color.y));
	obj.serialized_variables.insert_or_assign("color_b", std::to_string(mLight.color.z));
	obj.serialized_variables.insert_or_assign("type", std::to_string(mLight.type));
	obj.serialized_variables.insert_or_assign("override_dist", std::to_string(mLight.override_dist));
	obj.serialized_variables.insert_or_assign("dir_backtrack_dist", std::to_string(mLight.dir_backtrack_dist));
	obj.serialized_variables.insert_or_assign("angle_inner", std::to_string(mLight.angle_inner));
	obj.serialized_variables.insert_or_assign("angle_outer", std::to_string(mLight.angle_outer));
	obj.serialized_variables.insert_or_assign("near_clip", std::to_string(mLight.near_clip));
	obj.serialized_variables.insert_or_assign("range", std::to_string(mLight.range));
	obj.serialized_variables.insert_or_assign("bias_min", std::to_string(mLight.bias_min));
	obj.serialized_variables.insert_or_assign("bias_mult", std::to_string(mLight.bias_mult));

	return obj;
}

gbe::LightObject::LightObject(SerializedObject* data) : Object(data)
{
	mLight.color.x = std::stof(data->serialized_variables["color_r"]);
	mLight.color.y = std::stof(data->serialized_variables["color_g"]);
	mLight.color.z = std::stof(data->serialized_variables["color_b"]);
	mLight.type = (gfx::Light::LightType)std::stoi(data->serialized_variables["type"]);
	mLight.override_dist = std::stof(data->serialized_variables["override_dist"]);
	mLight.dir_backtrack_dist = std::stof(data->serialized_variables["dir_backtrack_dist"]);
	mLight.angle_inner = std::stof(data->serialized_variables["angle_inner"]);
	mLight.angle_outer = std::stof(data->serialized_variables["angle_outer"]);
	mLight.near_clip = std::stof(data->serialized_variables["near_clip"]);
	mLight.range = std::stof(data->serialized_variables["range"]);
	mLight.bias_min = std::stof(data->serialized_variables["bias_min"]);
	mLight.bias_mult = std::stof(data->serialized_variables["bias_mult"]);
}

void gbe::LightObject::InitializeInspectorData()
{
	Object::InitializeInspectorData();

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
