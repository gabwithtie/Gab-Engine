#include "BuilderBlock.h"

#include <vector>

namespace gbe::ext::AnitoBuilder {
	BuilderBlock::BuilderBlock(gbe::Vector3 corners[4], float height)
	{
		this->SetName("Anito Builder Block");

		this->height = height;
		auto base_center = Vector3::Mid(Vector3::Mid(corners[0], corners[1]), Vector3::Mid(corners[2], corners[3]));

		this->segments = {
			{corners[0], corners[1]},
			{corners[1], corners[2]},
			{corners[2], corners[3]},
			{corners[3], corners[0]},
		};

		for (size_t i = 0; i < segments.size(); i++)
		{
			const auto& seg = segments[i];

			auto handle = new RigidObject(true);
			auto collider = new BoxCollider();
			collider->SetParent(handle);
			auto renderer = new RenderObject(RenderObject::cube);
			renderer->SetParent(handle);

			handle->SetParent(this);

			Vector3 delta = seg.first - seg.second;
			float half_mag = delta.Magnitude() * 0.5f;

			handle->Local().position.Set(Vector3::Mid(seg.first, seg.second) + Vector3(0, height * 0.5f, 0));
			handle->Local().rotation.Set(Quaternion::LookAtRotation(delta.Cross(Vector3::Up()).Normalize(), Vector3::Up()));
			handle->Local().scale.Set(Vector3(half_mag, height * 0.5f, 0.01f));

			handle->PushEditorFlag(Object::EditorFlags::STATIC_POS_Y);
			handle->PushEditorFlag(Object::EditorFlags::STATIC_ROT_X);
			handle->PushEditorFlag(Object::EditorFlags::STATIC_ROT_Z);
			handle->PushEditorFlag(Object::EditorFlags::STATIC_SCALE_Y);
			handle->PushEditorFlag(Object::EditorFlags::STATIC_SCALE_Z);
			handle->PushEditorFlag(Object::EditorFlags::IS_STATE_MANAGED);

			this->handles.push_back(handle);
		}
	}

	void BuilderBlock::RecalculateTransformations(int adjusted_seg) {
		for (size_t i = 0; i < segments.size(); i++)
		{
			if (adjusted_seg == i) //Do not recalculate user-edited anymore.
				continue;

			const auto& seg = segments[i];
			auto handle = handles[i];

			Vector3 delta = seg.first - seg.second;
			float half_mag = delta.Magnitude() * 0.5f;

			handle->Local().position.Set(Vector3::Mid(seg.first, seg.second) + Vector3(0, height * 0.5f, 0));
			handle->Local().rotation.Set(Quaternion::LookAtRotation(delta.Cross(Vector3::Up()).Normalize(), Vector3::Up()));
			handle->Local().scale.Set(Vector3(half_mag, height * 0.5f, 0.01f));
		}
	}

	void BuilderBlock::InvokeUpdate(float deltatime) {
		int adjusted_seg = -1;

		for (size_t i = 0; i < handles.size(); i++)
		{
			const auto& handle = handles[i];

			bool moved = handle->CheckState(Object::ObjectStateName::TRANSFORMED_USER, this);

			if (moved)
				adjusted_seg = i;
		}

		if (adjusted_seg >= 0) {
			auto handle = handles[adjusted_seg];
			auto& seg = segments[adjusted_seg];

			auto delta_right = handle->Local().GetRight() * (handle->Local().scale.Get().x);
			auto delta_up = -Vector3(0, height * 0.5f, 0);

			seg.first = handle->Local().position.Get() - delta_right + delta_up;
			seg.second = handle->Local().position.Get() + delta_right + delta_up;

			segments[adjusted_seg] = seg;
			GetSegment(adjusted_seg - 1).second = seg.first;
			GetSegment(adjusted_seg + 1).first = seg.second;

			RecalculateTransformations(adjusted_seg);
		}
	}
}
