#include "BuilderBlock.h"

#include <vector>

#include "BuilderBlockSet.h"

namespace gbe::ext::AnitoBuilder {
	BuilderBlock::BuilderBlock(gbe::Vector3 corners[4], float height)
		: Object(), Update()
	{
		this->SetName("Anito Builder Block");

		this->height = height;

		for (size_t i = 0; i < 4; i++)
		{
			this->position_pool.push_back(corners[i]);
		}

		int corner_ptrs[4] = {
			0,
			1,
			2,
			3,
		};

		AddBlock(corner_ptrs);
	}

	void BuilderBlock::UpdateHandleSegment(int s, int i, Vector3& l, Vector3& r)
	{
		i %= this->sets[s].size();

		auto& seg = this->sets[s][i].first;
		auto handle = this->sets[s][i].second;

		auto delta_right = handle->Local().GetRight() * (handle->Local().scale.Get().x);
		auto delta_up = -Vector3(0, height * 0.5f, 0);

		l = handle->Local().position.Get() - delta_right + delta_up;
		r = handle->Local().position.Get() + delta_right + delta_up;
	}

	void BuilderBlock::ResetHandle(int s, int i) {
		auto& seg = this->sets[s][i].first;
		auto handle = this->sets[s][i].second;

		Vector3 delta = position_pool[seg.first] - position_pool[seg.second];
		float half_mag = delta.Magnitude() * 0.5f;

		handle->Local().position.Set(Vector3::Mid(position_pool[seg.first], position_pool[seg.second]) + Vector3(0, height * 0.5f, 0));
		handle->Local().rotation.Set(Quaternion::LookAtRotation(delta.Cross(Vector3::Up()).Normalize(), Vector3::Up()));
		handle->Local().scale.Set(Vector3(half_mag, height * 0.5f, 0.01f));
	}

	inline bool BuilderBlock::CheckSetSegment(int s, int i, int point_index, Vector3 newpoint_a, Vector3 newpoint_b)
	{
		i %= this->sets[s].size();

		auto& seg = this->sets[s][i].first;
		auto handle = this->sets[s][i].second;

		Vector3 checker_l;
		Vector3 checker_r;

		if (point_index == 0) {
			checker_l = newpoint_b;
			checker_r = this->position_pool[seg.second];
		}
		else {
			checker_l = this->position_pool[seg.first];
			checker_r = newpoint_b;
		}

		Vector3 delta_a = checker_l - newpoint_a;
		Vector3 delta_b = checker_r - newpoint_a;

		auto anglebetween = Vector3::AngleBetween(delta_a, delta_b);


		if (anglebetween >= 135)
			return false;
		if (anglebetween <= 45)
			return false;
		if (delta_a.SqrMagnitude() < min_dist * min_dist)
			return false;
		if (delta_b.SqrMagnitude() < min_dist * min_dist)
			return false;
		if (delta_a.SqrMagnitude() > max_dist * max_dist)
			return false;
		if (delta_b.SqrMagnitude() > max_dist * max_dist)
			return false;

		return true;
	}

	inline void BuilderBlock::SetSegment(int s, int i, int point_index, Vector3 newpoint)
	{
		i %= this->sets[s].size();

		auto& seg = this->sets[s][i].first;
		auto handle = this->sets[s][i].second;

		Vector3* to_assign;

		if (point_index == 0) {
			to_assign = &this->position_pool[seg.first];
		}
		else {
			to_assign = &this->position_pool[seg.second];
		}


		*to_assign = newpoint;
	}

	void BuilderBlock::InvokeUpdate(float deltatime) {
		BuilderBlockSet* moved_obj = nullptr;
		int moved_pos_l = -1;
		int moved_pos_r = -1;

		Vector3 updated_l;
		Vector3 updated_r;

		int moved_s = -1;
		int moved_i = -1;

		for (size_t s = 0; s < sets.size(); s++) {
			for (size_t i = 0; i < sets[s].size(); i++)
			{
				auto& l = GetHandle(s, i);
				bool moved = l.second->CheckState(Object::ObjectStateName::TRANSFORMED_USER, this);

				if (moved) {
					moved_obj = l.second;
					UpdateHandleSegment(s, i, updated_l, updated_r);
					moved_pos_l = l.first.first;
					moved_pos_r = l.first.second;
					moved_s = s;
					moved_i = i;
					break;
				}
				else {
					ResetHandle(s, i);
				}
			}

			if (moved_obj != nullptr)
				break;
		}

		if (moved_obj == nullptr) //nothing more to do if theres nothing that moved
			return;

		bool valid_move = true;

		for (size_t s = 0; s < sets.size(); s++) {
			for (size_t i = 0; i < sets[s].size(); i++)
			{
				auto& l = GetHandle(s, i);

				if (l.second == moved_obj)
					continue;

				if (l.first.first == moved_pos_l)
					valid_move = valid_move && CheckSetSegment(s, i, 0, updated_l, updated_r);
				if (l.first.first == moved_pos_r)
					valid_move = valid_move && CheckSetSegment(s, i, 0, updated_r, updated_l);
				if (l.first.second == moved_pos_l)
					valid_move = valid_move && CheckSetSegment(s, i, 1, updated_l, updated_r);
				if (l.first.second == moved_pos_r)
					valid_move = valid_move && CheckSetSegment(s, i, 1, updated_r, updated_l);
			}
		}

		auto opp_pos = this->GetHandle(moved_s, moved_i + 2).second->Local().position.Get(); //The handle opposite of the moved handle
		Vector3 opp_dir = opp_pos - moved_obj->Local().position.Get();
		auto opp_dot = opp_dir.Dot(moved_obj->Local().GetForward());

		if (opp_dot < 0)
			valid_move = false;

		if (valid_move) {
			SetPosition(moved_pos_l, updated_l);
			SetPosition(moved_pos_r, updated_r);
		}
		else {
			return;
		}

		for (size_t s = 0; s < sets.size(); s++) {
			for (size_t i = 0; i < sets[s].size(); i++)
			{
				auto& l = GetHandle(s, i);

				if (l.second == moved_obj)
					continue;

				ResetHandle(s, i);
			}
		}
	}

	void BuilderBlock::AddBlock(BuilderBlockSet* root_handle) {
		SetSeg src_seg;

		for (size_t s = 0; s < sets.size(); s++)
			for (size_t i = 0; i < sets[s].size(); i++)
			{
				const auto& handle = sets[s][i].second;

				if (handle == root_handle) {
					src_seg = sets[s][i].first;
				}
			}

		Vector3 seg_dir = this->position_pool[src_seg.second] - this->position_pool[src_seg.first];
		Vector3 seg_3rd_dir = seg_dir.Cross(Vector3::Up()).Normalize() * 4.0f;

		int poolindex = position_pool.size();
		this->position_pool.push_back(this->position_pool[src_seg.second] - seg_3rd_dir);
		this->position_pool.push_back(this->position_pool[src_seg.first] - seg_3rd_dir);

		std::vector<int> corners = {
			src_seg.second,
			src_seg.first,
			poolindex + 1,
			poolindex,
		};

		AddBlock(corners.data(), root_handle);
	}

	void BuilderBlock::AddBlock(int corners[4], BuilderBlockSet* root_handle) {

		std::vector<SetSeg> src_segments = {
			{corners[0], corners[1]},
			{corners[1], corners[2]},
			{corners[2], corners[3]},
			{corners[3], corners[0]},
		};

		auto base_center = Vector3::Mid(Vector3::Mid(position_pool[corners[0]], position_pool[corners[1]]), Vector3::Mid(position_pool[corners[2]], position_pool[corners[3]]));
		
		std::vector<BlockSet> newset;

		int starting_index = 0;

		if (root_handle != nullptr) {
			newset.push_back({ {corners[0], corners[1]}, root_handle });
			starting_index = 1;
		}

		for (size_t i = starting_index; i < src_segments.size(); i++)
		{
			const auto& src_seg = src_segments[i];

			auto handle = new BuilderBlockSet(this);
			
			handle->SetParent(this);

			handle->PushEditorFlag(Object::EditorFlags::STATIC_POS_Y);
			handle->PushEditorFlag(Object::EditorFlags::STATIC_ROT_X);
			handle->PushEditorFlag(Object::EditorFlags::STATIC_ROT_Z);
			handle->PushEditorFlag(Object::EditorFlags::STATIC_SCALE_Y);
			handle->PushEditorFlag(Object::EditorFlags::STATIC_SCALE_Z);
			handle->PushEditorFlag(Object::EditorFlags::IS_STATE_MANAGED);
			
			
			newset.push_back({ src_seg, handle });

			Vector3 delta = position_pool[src_seg.first] - position_pool[src_seg.second];
			float half_mag = delta.Magnitude() * 0.5f;

			handle->Local().position.Set(Vector3::Mid(position_pool[src_seg.first], position_pool[src_seg.second]) + Vector3(0, height * 0.5f, 0));
			handle->Local().rotation.Set(Quaternion::LookAtRotation(delta.Cross(Vector3::Up()).Normalize(), Vector3::Up()));
			handle->Local().scale.Set(Vector3(half_mag, height * 0.5f, 0.01f));
		}

		this->sets.push_back(newset);
	}
}
