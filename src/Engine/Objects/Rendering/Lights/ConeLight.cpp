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

void gbe::ConeLight::GeneralInit()
{
    LightObject::GeneralInit();

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
    {
        auto field = new gbe::editor::InspectorFloat();
        field->name = "Inner Angle";
        field->getter = [=]() { return this->mLight.angle_inner_deg; };
        field->setter = [=](float val) { this->mLight.angle_inner_deg = val; };

        this->inspectorData->fields.push_back(field);
    }

    {
        auto field = new gbe::editor::InspectorFloat();
        field->name = "Outer Angle";
        field->getter = [=]() { return this->mLight.angle_outer_deg; };
        field->setter = [=](float val) { this->mLight.angle_outer_deg = val; };

        this->inspectorData->fields.push_back(field);
    }

    {
        auto field = new gbe::editor::InspectorFloat();
        field->name = "Range";
        field->getter = [=]() { return this->mLight.range; };
        field->setter = [=](float val) { this->mLight.range = val; };

        this->inspectorData->fields.push_back(field);
    }

    {
        auto field = new gbe::editor::InspectorFloat();
        field->name = "Near Clip";
        field->getter = [=]() { return this->mLight.near_clip; };
        field->setter = [=](float val) { this->mLight.near_clip = val; };

        this->inspectorData->fields.push_back(field);
    }
}