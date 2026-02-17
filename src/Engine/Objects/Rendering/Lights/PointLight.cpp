#include "PointLight.h"

#include "Engine/Objects/Physics/RigidObject.h"
#include "Engine/Objects/Physics/Collider/BoxCollider.h"

#include "Engine/Objects/Rendering/RenderObject.h"

#include "Engine/Engine.h"

#include "Editor/gbe_editor.h"

#include <array>

gbe::gfx::Light* gbe::PointLight::GetData()
{
    this->mLight.direction = this->World().GetForward();
    this->mLight.type = Light::POINT;
    this->mLight.position = this->World().position.Get();

    return &this->mLight;
}

void gbe::PointLight::GeneralInit()
{
    LightObject::GeneralInit();

    this->PushEditorFlag(Object::EditorFlags::SERIALIZABLE);

    this->mLight.bias_min = 0.005;
    this->mLight.bias_mult = 0.05;

    if (Engine::Get_state() == Engine::EngineState::Edit) {
        auto gui_drawcall = RenderPipeline::RegisterDrawCall(asset::Mesh::GetAssetById("sphere"), asset::Material::GetAssetById("wireframe"));
        auto light_gizmo = new RenderObject(gui_drawcall);
        light_gizmo->SetParent(this);
        light_gizmo->PushEditorFlag(Object::SELECT_PARENT_INSTEAD);
        light_gizmo->PushEditorFlag(Object::EXCLUDE_FROM_OBJECT_TREE);
    }

    //INSPECTOR

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