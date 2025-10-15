#pragma once

#include "LightObject.h"

namespace gbe {
	class DirectionalLight : public gbe::LightObject {
	public:
		inline DirectionalLight()
		{
			InitializeInspectorData();
		}

		inline DirectionalLight(SerializedObject* data) : LightObject(data)
		{
			InitializeInspectorData();
		}
		
		// Inherited via Light
		virtual gfx::Light* GetData() override;
		void InitializeInspectorData() override;
	};
}