#pragma once

#include "LightObject.h"

namespace gbe {
	class DirectionalLight : public gbe::LightObject {
	public:
		inline DirectionalLight()
		{
			GeneralInit();
		}

		inline DirectionalLight(SerializedObject* data) : LightObject(data)
		{
			GeneralInit();
		}
		
		// Inherited via Light
		virtual gfx::Light* GetData() override;
		void GeneralInit() override;
	};
}