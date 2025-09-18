#pragma once

#include "LightObject.h"

namespace gbe {
	class DirectionalLight : public gbe::LightObject {
	public:
		DirectionalLight();
		// Inherited via Light
		virtual gfx::Light* GetData() override;
	};
}