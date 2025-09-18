#include "DirectionalLight.h"

#include "Engine/Objects/Physics/RigidObject.h"
#include "Engine/Objects/Rendering/RenderObject.h"

using namespace gbe;
using namespace gbe::gfx;

gfx::Light* gbe::DirectionalLight::GetData()
{
    this->mLight.direction = this->World().GetForward();
    this->mLight.color = Vector3(1, 1, 0.7f);
    this->mLight.type = Light::DIRECTIONAL;
    this->mLight.position = this->World().position.Get();

    return &this->mLight;
}

void gbe::DirectionalLight::InitializeEditorSubObjects()
{
	auto dirlight_ro = new RigidObject(true);
	dirlight_ro->SetParent(this);
	dirlight_ro->PushEditorFlag(Object::SELECT_PARENT_INSTEAD);
	dirlight_ro->PushEditorFlag(Object::EXCLUDE_FROM_OBJECT_TREE);

	auto arrow_drawcall = RenderPipeline::RegisterDrawCall(asset::Mesh::GetAssetById("arrow"), asset::Material::GetAssetById("wireframe"));
	auto dirlight_gizmo = new RenderObject(arrow_drawcall);
	dirlight_gizmo->SetParent(dirlight_ro);
	dirlight_gizmo->Local().position.Set(Vector3(0, 0, 1.0f));
	dirlight_gizmo->Local().scale.Set(Vector3(0.2, 0.2, -1.0f));
}