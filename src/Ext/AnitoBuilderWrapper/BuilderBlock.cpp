#include "BuilderBlock.h"

#include <vector>

#include "BuilderBlockSet.h"

namespace gbe::ext::AnitoBuilder {
	BuilderBlock::BuilderBlock(gbe::Vector3 corners[4], float height)
		: Object(), Update()
	{
		this->SetName("Anito Builder Block");

		this->height = height;
		
		AddBlock(corners);
	}

	void BuilderBlock::RecalculateTransformations(BuilderBlockSet* adjusted) {
		for (size_t s = 0; s < this->sets.size(); s++)
		{
			const auto& segments = this->sets[s];

			for (size_t i = 0; i < segments.size(); i++)
			{
				if (adjusted == handles[s][i]) //Do not recalculate user-edited anymore.
					continue;

				const auto& seg = segments[i];
				auto handle = handles[s][i];

				Vector3 delta = seg.first - seg.second;
				float half_mag = delta.Magnitude() * 0.5f;

				handle->Local().position.Set(Vector3::Mid(seg.first, seg.second) + Vector3(0, height * 0.5f, 0));
				handle->Local().rotation.Set(Quaternion::LookAtRotation(delta.Cross(Vector3::Up()).Normalize(), Vector3::Up()));
			}
		}
	}

	void BuilderBlock::InvokeUpdate(float deltatime) {
		bool moved_any = false;
		BuilderBlockSet* moved_obj = nullptr;

		for (size_t s = 0; s < handles.size(); s++)
			for (size_t i = 0; i < handles[s].size(); i++)
			{
				const auto& handle = handles[s][i];

				bool moved = handle->CheckState(Object::ObjectStateName::TRANSFORMED_USER, this);

				if (moved) {
					moved_any = true;

					if (moved_obj == nullptr)
						moved_obj = handle;
					else if (moved_obj != handle)
						throw new std::runtime_error("Moving two handles at the same time is unsupported.");

					auto& seg = sets[s][i];

					auto delta_right = handle->Local().GetRight() * (handle->Local().scale.Get().x);
					auto delta_up = -Vector3(0, height * 0.5f, 0);

					seg.first = handle->Local().position.Get() - delta_right + delta_up;
					seg.second = handle->Local().position.Get() + delta_right + delta_up;

					sets[s][i] = seg;
					GetSegment(s, i - 1).second = seg.first;
					GetSegment(s, i + 1).first = seg.second;
				}
			}

		if (moved_any)
			RecalculateTransformations(moved_obj);
	}

	void BuilderBlock::AddBlock(BuilderBlockSet* root_handle) {
		int src_set;
		std::pair<Vector3, Vector3> src_seg;

		for (size_t s = 0; s < handles.size(); s++)
			for (size_t i = 0; i < handles[s].size(); i++)
			{
				const auto& handle = handles[s][i];

				if (handle == root_handle) {
					src_set = s;
					src_seg = sets[s][i];
				}
			}

		Vector3 seg_dir = src_seg.second - src_seg.first;
		Vector3 seg_3rd_dir = seg_dir.Cross(Vector3::Up()).Normalize();

		std::vector<Vector3> corners = {
			src_seg.first,
			src_seg.second,
			src_seg.second + seg_3rd_dir,
			src_seg.first + seg_3rd_dir,
		};

		AddBlock(corners.data(), root_handle);
	}

	void BuilderBlock::AddBlock(Vector3 corners[4], BuilderBlockSet* root_handle) {
		auto base_center = Vector3::Mid(Vector3::Mid(corners[0], corners[1]), Vector3::Mid(corners[2], corners[3]));

		std::vector<std::pair<Vector3, Vector3>> segments = {
			{corners[0], corners[1]},
			{corners[1], corners[2]},
			{corners[2], corners[3]},
			{corners[3], corners[0]},
		};
		std::vector<BuilderBlockSet*> handle_set;

		int starting_index = 0;

		if (root_handle != nullptr) {
			handle_set.push_back(root_handle);
			starting_index = 1;
		}

		for (size_t i = starting_index; i < segments.size(); i++)
		{
			const auto& seg = segments[i];

			auto handle = new BuilderBlockSet(this);
			auto handle_ro = new RigidObject(true);
			handle_ro->SetParent(handle);
			auto collider = new BoxCollider();
			collider->SetParent(handle_ro);
			auto renderer = new RenderObject(RenderObject::cube);
			renderer->SetParent(handle_ro);

			handle->SetParent(this);

			handle->PushEditorFlag(Object::EditorFlags::STATIC_POS_Y);
			handle->PushEditorFlag(Object::EditorFlags::STATIC_ROT_X);
			handle->PushEditorFlag(Object::EditorFlags::STATIC_ROT_Z);
			handle->PushEditorFlag(Object::EditorFlags::STATIC_SCALE_Y);
			handle->PushEditorFlag(Object::EditorFlags::STATIC_SCALE_Z);
			handle->PushEditorFlag(Object::EditorFlags::IS_STATE_MANAGED);
			
			handle_set.push_back(handle);

			Vector3 delta = seg.first - seg.second;
			float half_mag = delta.Magnitude() * 0.5f;

			handle->Local().position.Set(Vector3::Mid(seg.first, seg.second) + Vector3(0, height * 0.5f, 0));
			handle->Local().rotation.Set(Quaternion::LookAtRotation(delta.Cross(Vector3::Up()).Normalize(), Vector3::Up()));
			handle->Local().scale.Set(Vector3(half_mag, height * 0.5f, 0.01f));
		}

		this->sets.push_back(segments);
		this->handles.push_back(handle_set);
	}
}
