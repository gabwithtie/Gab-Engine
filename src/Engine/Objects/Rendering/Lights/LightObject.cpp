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
	obj.serialized_variables.insert_or_assign("angle_inner_deg", std::to_string(mLight.angle_inner_deg));
	obj.serialized_variables.insert_or_assign("angle_outer_deg", std::to_string(mLight.angle_outer_deg));
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
	mLight.angle_inner_deg = std::stof(data->serialized_variables["angle_inner_deg"]);
	mLight.angle_outer_deg = std::stof(data->serialized_variables["angle_outer_deg"]);
	mLight.near_clip = std::stof(data->serialized_variables["near_clip"]);
	mLight.range = std::stof(data->serialized_variables["range"]);
	mLight.bias_min = std::stof(data->serialized_variables["bias_min"]);
	mLight.bias_mult = std::stof(data->serialized_variables["bias_mult"]);
}

void gbe::LightObject::GeneralInit()
{
	Object::GeneralInit();

	{
		auto field = new gbe::editor::InspectorColor();
		field->name = "Color";
		field->getter = [=]() {return this->mLight.color; };
		field->setter = [=](Vector3 val) {this->mLight.color = val; };

		this->inspectorData->fields.push_back(field);
	}

	{
		auto field = new gbe::editor::InspectorFloat();
		field->name = "Bias min";
		field->getter = [=]() {return this->mLight.bias_min; };
		field->setter = [=](float val) {this->mLight.bias_min = val; };

		this->inspectorData->fields.push_back(field);
	}
	{
		auto field = new gbe::editor::InspectorFloat();
		field->name = "Bias mult";
		field->getter = [=]() {return this->mLight.bias_mult; };
		field->setter = [=](float val) {this->mLight.bias_mult = val; };

		this->inspectorData->fields.push_back(field);
	}
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

void gbe::LightObject::InvokeUpdate(float deltatime)
{
	if (Engine::Get_state() == Engine::EngineState::Edit) {
		const auto& cam = Engine::GetActiveCamera();
		editor::ViewportWindow::RenderIcon(TextureLoader::GetAssetRuntimeData("lights"), this->World().position.Get(), cam->GetViewMat(), cam->GetProjectionMat());
	}
}
