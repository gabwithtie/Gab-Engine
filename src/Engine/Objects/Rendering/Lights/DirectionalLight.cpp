#include "DirectionalLight.h"

using namespace gbe;
using namespace gbe::gfx;

gfx::Light* gbe::DirectionalLight::GetData()
{
    return &this->mLight;
}
