#include "ConeLight.h"

#include "Engine/Objects/Physics/RigidObject.h"
#include "Engine/Objects/Physics/Collider/BoxCollider.h"

#include "Engine/Objects/Rendering/RenderObject.h"

#include "Engine/Engine.h"

#include "Editor/gbe_editor.h"

using namespace gbe;
using namespace gbe::gfx;

gfx::Light* gbe::ConeLight::GetData()
{
    this->mLight.direction = this->World().GetForward();
    this->mLight.type = Light::CONE;
    this->mLight.position = this->World().position.Get();

    return &this->mLight;
}

void gbe::ConeLight::InitializeInspectorData()
{
	LightObject::InitializeInspectorData();

    this->PushEditorFlag(Object::EditorFlags::SERIALIZABLE);

    this->mLight.bias_min = 0.001;
    this->mLight.bias_mult = 0.01;

    if (Engine::Get_state() == Engine::EngineState::Edit) {
        auto arrow_drawcall = RenderPipeline::RegisterDrawCall(asset::Mesh::GetAssetById("arrow"), asset::Material::GetAssetById("wireframe"));
        auto dirlight_gizmo = new RenderObject(arrow_drawcall);
        dirlight_gizmo->SetParent(this);
        dirlight_gizmo->PushEditorFlag(Object::SELECT_PARENT_INSTEAD);
        dirlight_gizmo->PushEditorFlag(Object::EXCLUDE_FROM_OBJECT_TREE);
        dirlight_gizmo->Local().position.Set(Vector3(0, 0, 1.0f));
        dirlight_gizmo->Local().scale.Set(Vector3(0.2, 0.2, -1.0f));
    }

    //INSPECTOR
    auto angleinner_field = new gbe::editor::InspectorFloat();
    angleinner_field->name = "Inner Angle";
    angleinner_field->x = &this->mLight.angle_inner_deg;

    this->inspectorData->fields.push_back(angleinner_field);

    auto angleouter_field = new gbe::editor::InspectorFloat();
    angleouter_field->name = "Outer Angle";
    angleouter_field->x = &this->mLight.angle_outer_deg;

    this->inspectorData->fields.push_back(angleouter_field);

    auto range_field = new gbe::editor::InspectorFloat();
    range_field->name = "Range";
    range_field->x = &this->mLight.range;

    this->inspectorData->fields.push_back(range_field);

    auto near_field = new gbe::editor::InspectorFloat();
    near_field->name = "Near Clip";
    near_field->x = &this->mLight.near_clip;

    this->inspectorData->fields.push_back(near_field);
}