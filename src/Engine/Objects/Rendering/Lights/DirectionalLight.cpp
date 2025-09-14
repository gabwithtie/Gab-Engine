#include "DirectionalLight.h"

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
