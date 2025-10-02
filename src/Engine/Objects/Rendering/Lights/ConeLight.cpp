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

gbe::ConeLight::ConeLight()
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
    auto angle_field = new gbe::editor::InspectorFloat();
    angle_field->name = "Angle";
    angle_field->x = &this->mLight.angle;

    this->inspectorData->fields.push_back(angle_field);

    auto range_field = new gbe::editor::InspectorFloat();
    range_field->name = "Range";
    range_field->x = &this->mLight.range;

    this->inspectorData->fields.push_back(range_field);

    auto lightcolor = new gbe::editor::InspectorColor();
    lightcolor->name = "Color";
    lightcolor->r = &this->mLight.color.x;
    lightcolor->g = &this->mLight.color.y;
    lightcolor->b = &this->mLight.color.z;

    this->inspectorData->fields.push_back(lightcolor);

}