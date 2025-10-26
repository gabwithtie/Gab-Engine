#pragma once

#include "Engine/Objects/Object.h"
#include "Graphics/gbe_graphics.h"

namespace gbe {

	class LightObject : public Object{
	protected:
		gfx::Light mLight;
		
		bool changed = true;

	public:
		LightObject(SerializedObject* data);

		inline void Set_noninstance_data(gfx::Light& data) {
			auto old_position = mLight.position;
			auto old_direction = mLight.direction;
			auto old_cam_view = mLight.cam_view;
			auto old_cam_proj = mLight.cam_proj;

			this->mLight = data;
			mLight.position = old_position;
			mLight.direction = old_direction;
			mLight.cam_view = old_cam_view;
			mLight.cam_proj = old_cam_proj;
		}
		void InitializeInspectorData() override;
		SerializedObject Serialize() override;

		LightObject();

		void Set_Color(Vector3 color);

		virtual gfx::Light* GetData() = 0;

		void OnLocalTransformationChange(TransformChangeType) override;
		void OnExternalTransformationChange(TransformChangeType, Matrix4 parentmat) override;
	};
}