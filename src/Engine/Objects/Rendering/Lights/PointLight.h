#pragma once

#include "LightObject.h"
#include "ConeLight.h"

namespace gbe {
	class PointLight : public LightObject {
	private:
		std::array<ConeLight*, 6> conelights;
	public:
		void Resync_sublights();
		inline PointLight() {
			InitializeInspectorData();
		}
		inline PointLight(SerializedObject* data) : LightObject(data) {
			InitializeInspectorData();
		}
		void InitializeInspectorData() override;

		inline virtual gfx::Light* GetData() override {
			return &mLight;
		}
	};
}