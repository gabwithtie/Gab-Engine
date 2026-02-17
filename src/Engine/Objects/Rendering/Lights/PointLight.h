#pragma once

#include "LightObject.h"
#include "ConeLight.h"

namespace gbe {
	class PointLight : public gbe::LightObject {
	public:
		inline PointLight() {
			GeneralInit();
		}
		inline PointLight(SerializedObject* data) : LightObject(data) {
			GeneralInit();
		}
		void GeneralInit() override;
		// Inherited via Light
		virtual gfx::Light* GetData() override;
	};
}