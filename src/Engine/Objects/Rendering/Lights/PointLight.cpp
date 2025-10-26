#include "PointLight.h"

#include "Engine/Objects/Physics/RigidObject.h"
#include "Engine/Objects/Physics/Collider/BoxCollider.h"

#include "Engine/Objects/Rendering/RenderObject.h"

#include "Engine/Engine.h"

#include "Editor/gbe_editor.h"

#include <array>

void gbe::PointLight::Resync_sublights()
{
    for (size_t i = 0; i < 6; i++)
    {
        conelights[i]->GetData()->color = this->mLight.color;
        conelights[i]->GetData()->bias_min = this->mLight.bias_min;
        conelights[i]->GetData()->bias_mult = this->mLight.bias_mult;
        conelights[i]->GetData()->near_clip = this->mLight.near_clip;
        conelights[i]->GetData()->range = this->mLight.range;

        conelights[i]->GetData()->angle_inner = 90;
        conelights[i]->GetData()->angle_outer = 90;
        conelights[i]->GetData()->square_project = true;
    }
}

void gbe::PointLight::InitializeInspectorData()
{
    LightObject::InitializeInspectorData();

    this->PushEditorFlag(Object::EditorFlags::SERIALIZABLE);

    this->mLight.bias_min = 0.005;
    this->mLight.bias_mult = 0.05;

    for (size_t i = 0; i < 6; i++)
    {
        auto newlight = new ConeLight();
        newlight->SetParent(this);
        newlight->PushEditorFlag(Object::EXCLUDE_FROM_OBJECT_TREE);
        newlight->PushEditorFlag(Object::SELECT_PARENT_INSTEAD);

        conelights[i] = newlight;
    }

    conelights[0]->Local().rotation.Set(Quaternion::LookAtRotation(Vector3(0, 0, 1), Vector3(0, 1, 0)));
    conelights[1]->Local().rotation.Set(Quaternion::LookAtRotation(Vector3(0, 0, -1), Vector3(0, 1, 0)));
    conelights[2]->Local().rotation.Set(Quaternion::LookAtRotation(Vector3(1, 0, 0), Vector3(0, 1, 0)));
    conelights[3]->Local().rotation.Set(Quaternion::LookAtRotation(Vector3(-1, 0, 0), Vector3(0, 1, 0)));
    conelights[4]->Local().rotation.Set(Quaternion::LookAtRotation(Vector3(0, 1, 0), Vector3(0, 0, 1)));
    conelights[5]->Local().rotation.Set(Quaternion::LookAtRotation(Vector3(0, -1, 0), Vector3(0, 0, 1)));

    if (Engine::Get_state() == Engine::EngineState::Edit) {
        auto light_ro = new RigidObject(true);
        light_ro->SetParent(this);
        light_ro->PushEditorFlag(Object::SELECT_PARENT_INSTEAD);
        light_ro->PushEditorFlag(Object::EXCLUDE_FROM_OBJECT_TREE);

        auto light_col = new SphereCollider();
        light_col->Local().scale.Set(Vector3(1.0f));
        light_col->SetParent(light_ro);

        auto gui_drawcall = RenderPipeline::RegisterDrawCall(asset::Mesh::GetAssetById("sphere"), asset::Material::GetAssetById("wireframe"));
        auto light_gizmo = new RenderObject(gui_drawcall);
        light_gizmo->SetParent(light_ro);
    }

    //INSPECTOR

    auto range_field = new gbe::editor::InspectorFloat();
    range_field->name = "Range";
    range_field->x = &this->mLight.range;

    this->inspectorData->fields.push_back(range_field);

    auto near_field = new gbe::editor::InspectorFloat();
    near_field->name = "Near Clip";
    near_field->x = &this->mLight.near_clip;

    this->inspectorData->fields.push_back(near_field);

    Resync_sublights();
}