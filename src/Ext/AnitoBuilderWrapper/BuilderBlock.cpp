#include "BuilderBlock.h"

#include <vector>

#include "BuilderBlockSet.h"
#include "Graphics/gbe_graphics.h"
#include "Asset/gbe_asset.h"
#include "Math/gbe_math.h"

namespace gbe::ext::AnitoBuilder {
	bool BuilderBlock::model_shown = false;

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

		//OBJECTS
		AddBlock(corner_ptrs);
		renderer_parent = new Object();
		renderer_parent->SetParent(this);

		//RENDERING
		Wall1_DC = RenderPipeline::RegisterDrawCall(asset::Mesh::GetAssetById("wall"), asset::Material::GetAssetById("lit"));
		Wall2_DC = RenderPipeline::RegisterDrawCall(asset::Mesh::GetAssetById("roof"), asset::Material::GetAssetById("lit"));

		//INSPECTOR
		auto height_field = new gbe::editor::InspectorFloat();
		height_field->name = "Height";
		height_field->x = &this->height;

		this->inspectorData->fields.push_back(height_field);
	}

	void BuilderBlock::UpdateModelShown()
	{
		if (!model_shown)
			for (size_t i = 0; i < renderer_parent->GetChildCount(); i++)
			{
				renderer_parent->GetChildAt(i)->Destroy();
			}

		if (model_shown)
			for (const auto& handle : this->handle_pool)
			{
				if (!handle->Get_is_edge())
					continue;

				for (const auto& obj : handle->Get_all_segs())
				{
					Vector3 pos = obj->World().position.Get();
					pos -= Vector3(0, handle->Get_height_per_wall() / 2.0f, 0);

					Quaternion rot = obj->World().rotation.Get();
					Quaternion flip_rot = Quaternion::Euler(Vector3(0, 180, 0));
					rot *= flip_rot;

					int floor_index = handle->Get_floor(obj);
					RenderObject* newrenderer = nullptr;

					if(floor_index == 0)
						newrenderer = new RenderObject(Wall1_DC);
					if(floor_index > 0)
						newrenderer = new RenderObject(Wall2_DC);

					newrenderer->SetParent(renderer_parent);
					newrenderer->World().position.Set(pos);
					newrenderer->World().rotation.Set(rot);
					newrenderer->SetShadowCaster();

					auto inv__import_scale = Vector3(1.0f / (wall_import_width / 2), 1.0f / wall_import_height_from_zero, 1);
					Vector3 final_scale = Vector3(1);
					final_scale.x = inv__import_scale.x * (handle->Get_width_per_wall() / 2.0f);
					final_scale.y = inv__import_scale.y * (handle->Get_height_per_wall());

					newrenderer->Local().scale.Set(final_scale);
				}
			}

		for (const auto& handle : this->handle_pool)
		{
			handle->Set_visible(!model_shown);
		}
	}

	void BuilderBlock::SetModelShown(bool value)
	{
		model_shown = value;

		Engine::GetCurrentRoot()->CallRecursively([](Object* obj) {
			BuilderBlock* builderobj = nullptr;
			builderobj = dynamic_cast<BuilderBlock*>(obj);

			if (builderobj != nullptr) {
				builderobj->UpdateModelShown();
			}

			});
	}

	void BuilderBlock::UpdateHandleSegment(int s, int i, Vector3& l, Vector3& r)
	{
		i %= this->sets[s].size();

		auto& seg = this->sets[s][i].setseg;
		auto handle = this->sets[s][i].block;

		auto delta_right = handle->Local().GetRight() * (handle->Local().scale.Get().x);
		auto delta_up = -Vector3(0, height * 0.5f, 0);

		l = handle->Local().position.Get() - delta_right + delta_up;
		r = handle->Local().position.Get() + delta_right + delta_up;
	}

	void BuilderBlock::ResetHandle(int s, int i) {
		auto& seg = this->sets[s][i].setseg;
		auto handle = this->sets[s][i].block;

		Vector3 delta = position_pool[seg.first] - position_pool[seg.second];
		float half_mag = delta.Magnitude() * 0.5f;

		handle->Local().position.Set(Vector3::Mid(position_pool[seg.first], position_pool[seg.second]) + Vector3(0, height * 0.5f, 0));
		handle->Local().rotation.Set(Quaternion::LookAtRotation(delta.Cross(Vector3::Up()).Normalize(), Vector3::Up()));
		handle->Local().scale.Set(Vector3(half_mag, height * 0.5f, 0.01f));
	}

	inline bool BuilderBlock::CheckSetSegment(int s, int i, int point_index, Vector3 newpoint_a, Vector3 newpoint_b)
	{
		i %= this->sets[s].size();

		auto& seg = this->sets[s][i].setseg;
		auto handle = this->sets[s][i].block;

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

		auto& seg = this->sets[s][i].setseg;
		auto handle = this->sets[s][i].block;

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
		if (model_shown)
			return; //no editing if models are shown

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
				bool moved = l.block->CheckState(Object::ObjectStateName::TRANSFORMED_USER, this);

				if (moved) {
					moved_obj = l.block;
					UpdateHandleSegment(s, i, updated_l, updated_r);
					moved_pos_l = l.setseg.first;
					moved_pos_r = l.setseg.second;
					moved_s = s;
					moved_i = i;
					break;
				}
				else if (l.block->CheckState(Object::ObjectStateName::TRANSFORMED_LOCAL, this)) {
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

				if (l.block == moved_obj)
					continue;

				if (l.setseg.first == moved_pos_l)
					valid_move = valid_move && CheckSetSegment(s, i, 0, updated_l, updated_r);
				if (l.setseg.first == moved_pos_r)
					valid_move = valid_move && CheckSetSegment(s, i, 0, updated_r, updated_l);
				if (l.setseg.second == moved_pos_l)
					valid_move = valid_move && CheckSetSegment(s, i, 1, updated_l, updated_r);
				if (l.setseg.second == moved_pos_r)
					valid_move = valid_move && CheckSetSegment(s, i, 1, updated_r, updated_l);
			}
		}

		auto opp_pos = this->GetHandle(moved_s, moved_i + 2).block->Local().position.Get(); //The handle opposite of the moved handle
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

				if (l.block == moved_obj)
					continue;

				ResetHandle(s, i);
			}
		}
	}

	void BuilderBlock::AddBlock(BuilderBlockSet* root_handle) {
		if (!root_handle->Get_is_edge())
			return;

		SetSeg src_seg;

		for (size_t s = 0; s < sets.size(); s++)
			for (size_t i = 0; i < sets[s].size(); i++)
			{
				const auto& handle = sets[s][i].block;

				if (handle == root_handle) {
					src_seg = sets[s][i].setseg;
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
			root_handle->Set_is_edge(false);
			newset.push_back({ {corners[0], corners[1]}, root_handle });
			starting_index = 1;
		}

		for (size_t i = starting_index; i < src_segments.size(); i++)
		{
			const auto& src_seg = src_segments[i];

			auto handle = new BuilderBlockSet(this);
			
			handle_pool.push_back(handle);
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
