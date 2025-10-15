#pragma once

#include "LightObject.h"

namespace gbe {
	class ConeLight : public gbe::LightObject {
	public:
		inline ConeLight() {
			InitializeInspectorData();
		}
		inline ConeLight(SerializedObject* data) : LightObject(data) {
			InitializeInspectorData();
		}
		void InitializeInspectorData() override;
		// Inherited via Light
		virtual gfx::Light* GetData() override;
	};
}