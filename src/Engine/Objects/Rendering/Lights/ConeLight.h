#pragma once

#include "LightObject.h"

namespace gbe {
	class ConeLight : public gbe::LightObject {
	public:
		ConeLight();
		// Inherited via Light
		virtual gfx::Light* GetData() override;
	};
}