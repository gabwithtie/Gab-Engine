#pragma once

#include "LightObject.h"

namespace gbe {
	class ConeLight : public gbe::LightObject {
	public:
		inline ConeLight() {
			GeneralInit();
		}
		inline ConeLight(SerializedObject* data) : LightObject(data) {
			GeneralInit();
		}
		void GeneralInit() override;
		// Inherited via Light
		virtual gfx::Light* GetData() override;
	};
}