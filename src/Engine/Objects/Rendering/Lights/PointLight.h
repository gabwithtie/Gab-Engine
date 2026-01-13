#pragma once

#include "LightObject.h"
#include "ConeLight.h"

namespace gbe {
	class PointLight : public gbe::LightObject {
	public:
		inline PointLight() {
			InitializeInspectorData();
		}
		inline PointLight(SerializedObject* data) : LightObject(data) {
			InitializeInspectorData();
		}
		void InitializeInspectorData() override;
		// Inherited via Light
		virtual gfx::Light* GetData() override;
	};
}