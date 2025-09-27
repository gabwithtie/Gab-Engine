#include "DirectionalLight.h"

#include "Engine/Objects/Physics/RigidObject.h"
#include "Engine/Objects/Physics/Collider/BoxCollider.h"

#include "Engine/Objects/Rendering/RenderObject.h"

#include "Engine/Engine.h"

#include "Editor/gbe_editor.h"

using namespace gbe;
using namespace gbe::gfx;

gfx::Light* gbe::DirectionalLight::GetData()
{
    this->mLight.direction = this->World().GetForward();
    this->mLight.type = Light::DIRECTIONAL;
    this->mLight.position = this->World().position.Get();

    return &this->mLight;
}

gbe::DirectionalLight::DirectionalLight()
{
    if (Engine::Get_state() == Engine::EngineState::Edit) {
        auto dirlight_ro = new RigidObject(true);
        dirlight_ro->SetParent(this);
        dirlight_ro->PushEditorFlag(Object::SELECT_PARENT_INSTEAD);
        dirlight_ro->PushEditorFlag(Object::EXCLUDE_FROM_OBJECT_TREE);

        auto dirlight_col = new BoxCollider();
        dirlight_col->Local().scale.Set(Vector3(0.5, 0.5, 1.0f));
        dirlight_col->Local().position.Set(Vector3(0, 0, 0.5f));
        dirlight_col->SetParent(dirlight_ro);

        auto arrow_drawcall = RenderPipeline::RegisterDrawCall(asset::Mesh::GetAssetById("arrow"), asset::Material::GetAssetById("wireframe"));
        auto dirlight_gizmo = new RenderObject(arrow_drawcall);
        dirlight_gizmo->SetParent(dirlight_ro);
        dirlight_gizmo->Local().position.Set(Vector3(0, 0, 1.0f));
        dirlight_gizmo->Local().scale.Set(Vector3(0.2, 0.2, -1.0f));
    }

    //INSPECTOR
    auto overshoot_field = new gbe::editor::InspectorFloat();
    overshoot_field->name = "Shadowmap overshoot";
    overshoot_field->x = &this->mLight.dir_overshoot_dist;

    this->inspectorData->fields.push_back(overshoot_field);

    auto backtrack_field = new gbe::editor::InspectorFloat();
    backtrack_field->name = "Shadowmap backtrack";
    backtrack_field->x = &this->mLight.dir_backtrack_dist;

    this->inspectorData->fields.push_back(backtrack_field);

    auto bias_min = new gbe::editor::InspectorFloat();
    bias_min->name = "Bias min";
    bias_min->x = &this->mLight.bias_min;

    this->inspectorData->fields.push_back(bias_min);

    auto bias_mult = new gbe::editor::InspectorFloat();
    bias_mult->name = "Bias mult";
    bias_mult->x = &this->mLight.bias_mult;

    this->inspectorData->fields.push_back(bias_mult);

}