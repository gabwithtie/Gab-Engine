#pragma once

#include "Engine/Objects/Object.h"
#include "Graphics/gbe_graphics.h"

namespace gbe {

	class LightObject : public Object{
	protected:
		gfx::Light mLight;
		
		bool changed = true;

	public:

		void Set_Color(Vector3 color);

		virtual gfx::Light* GetData() = 0;

		void OnLocalTransformationChange(TransformChangeType) override;
		void OnExternalTransformationChange(TransformChangeType, Matrix4 parentmat) override;
	};
}