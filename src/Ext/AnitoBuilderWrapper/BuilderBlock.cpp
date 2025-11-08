#include "BuilderBlock.h"

#include <vector>

#include "BuilderBlockSet.h"
#include "Graphics/gbe_graphics.h"
#include "Asset/gbe_asset.h"
#include "Math/gbe_math.h"

namespace gbe::ext::AnitoBuilder {
	bool BuilderBlock::model_shown = false;

	void BuilderBlock::LoadAssets()
	{
		auto material = asset::Material::GetAssetById("lit");

		//Editor
		ceiling_editor_DC = RenderPipeline::RegisterDrawCall(asset::Mesh::GetAssetById("horizontal_axis_triangle"), asset::Material::GetAssetById("grid"));
		
		//Main - normal
		ceiling_DC = RenderPipeline::RegisterDrawCall(asset::Mesh::GetAssetById("horizontal_axis_triangle"), material);
		
		roof_DC = RenderPipeline::RegisterDrawCall(asset::Mesh::GetAssetById("roof_1"), material);
		
		wallnorm_DC[0] = RenderPipeline::RegisterDrawCall(asset::Mesh::GetAssetById("wallnorm1"), material);
		wallnorm_DC[1] = RenderPipeline::RegisterDrawCall(asset::Mesh::GetAssetById("wallnorm2"), material);

		wall3x4_DC[0][0] = RenderPipeline::RegisterDrawCall(asset::Mesh::GetAssetById("3x4wall_1-1"), material);
		wall3x4_DC[0][1] = RenderPipeline::RegisterDrawCall(asset::Mesh::GetAssetById("3x4wall_1-2"), material);
		wall3x4_DC[0][2] = RenderPipeline::RegisterDrawCall(asset::Mesh::GetAssetById("3x4wall_1-3"), material);
		wall3x4_DC[1][0] = RenderPipeline::RegisterDrawCall(asset::Mesh::GetAssetById("3x4wall_2-1"), material);
		wall3x4_DC[1][1] = RenderPipeline::RegisterDrawCall(asset::Mesh::GetAssetById("3x4wall_2-2"), material);
		wall3x4_DC[1][2] = RenderPipeline::RegisterDrawCall(asset::Mesh::GetAssetById("3x4wall_2-3"), material);
		wall3x4_DC[2][0] = RenderPipeline::RegisterDrawCall(asset::Mesh::GetAssetById("3x4wall_3-1"), material);
		wall3x4_DC[2][1] = RenderPipeline::RegisterDrawCall(asset::Mesh::GetAssetById("3x4wall_3-2"), material);
		wall3x4_DC[2][2] = RenderPipeline::RegisterDrawCall(asset::Mesh::GetAssetById("3x4wall_3-3"), material);
		wall3x4_DC[3][0] = RenderPipeline::RegisterDrawCall(asset::Mesh::GetAssetById("3x4wall_4-1"), material);
		wall3x4_DC[3][1] = RenderPipeline::RegisterDrawCall(asset::Mesh::GetAssetById("3x4wall_4-2"), material);
		wall3x4_DC[3][2] = RenderPipeline::RegisterDrawCall(asset::Mesh::GetAssetById("3x4wall_4-3"), material);

		wall2x3_DC[0][0] = RenderPipeline::RegisterDrawCall(asset::Mesh::GetAssetById("2x3wall_1-1"), material);
		wall2x3_DC[0][1] = RenderPipeline::RegisterDrawCall(asset::Mesh::GetAssetById("2x3wall_1-2"), material);
		wall2x3_DC[1][0] = RenderPipeline::RegisterDrawCall(asset::Mesh::GetAssetById("2x3wall_2-1"), material);
		wall2x3_DC[1][1] = RenderPipeline::RegisterDrawCall(asset::Mesh::GetAssetById("2x3wall_2-2"), material);
		wall2x3_DC[2][0] = RenderPipeline::RegisterDrawCall(asset::Mesh::GetAssetById("2x3wall_3-1"), material);
		wall2x3_DC[2][1] = RenderPipeline::RegisterDrawCall(asset::Mesh::GetAssetById("2x3wall_3-2"), material);
	}

	BuilderBlock::BuilderBlock(gbe::Vector3 corners[4], float height)
		: Object(), Update()
	{
		this->SetName("Anito Builder Block");
		
		LoadAssets();

		//OBJECTS
		this->height = height;

		for (size_t i = 0; i < 4; i++)
		{
			this->data.AddPosition(corners[i]);
		}

		int corner_ptrs[4] = {
			0,
			1,
			2,
			3,
		};

		AddBlock(corner_ptrs);

		InitializeInspectorData();
	}

	void BuilderBlock::InitializeInspectorData()
	{
		renderer_parent = new Object();
		renderer_parent->SetParent(this);

		Object::InitializeInspectorData();

		this->PushEditorFlag(Object::EditorFlags::SERIALIZABLE);

		//INSPECTOR
		{
			auto field = new gbe::editor::InspectorFloat();
			field->name = "Height";
			field->x = &this->height;

			this->inspectorData->fields.push_back(field);
		}
		{
			auto field = new gbe::editor::InspectorFloat();
			field->name = "segment_max_width";
			field->x = &this->wall_max_width;

			this->inspectorData->fields.push_back(field);
		}
		{
			auto field = new gbe::editor::InspectorFloat();
			field->name = "segment_max_height";
			field->x = &this->wall_max_height;

			this->inspectorData->fields.push_back(field);
		}
		{
			auto field = new gbe::editor::InspectorBool();
			field->name = "Allow multi-segments";
			field->x = &this->multisegment_models;

			this->inspectorData->fields.push_back(field);
		}
	}

	SerializedObject BuilderBlock::Serialize()
	{
		auto obj = Object::Serialize();

		auto data = gbe::asset::serialization::gbeParser::ExportClassStr(this->data);
		obj.serialized_variables.insert_or_assign("data", data);
		obj.serialized_variables.insert_or_assign("height", std::to_string(this->height));

		return obj;
	}

	BuilderBlock::BuilderBlock(SerializedObject* data) : Object(data)
	{
		this->SetName("Anito Builder Block");
		
		LoadAssets();

		BuilderBlockData newdata;
		gbe::asset::serialization::gbeParser::PopulateClassStr(newdata, data->serialized_variables["data"]);

		for (const auto& pos : newdata.positions)
		{
			this->data.AddPosition({ pos[0], pos[1] , pos[2] });
		}

		//OBJECTS
		this->height = std::stof(data->serialized_variables["height"]);

		for (const auto& set : newdata.sets)
		{
			int corner_ptrs[4] = {
				set.segs[0].seg.first,
				set.segs[1].seg.first,
				set.segs[2].seg.first,
				set.segs[3].seg.first
			};

			this->AddBlock(corner_ptrs);
		}

		InitializeInspectorData();
	}

	void BuilderBlock::UpdateModelShown()
	{
		if (!model_shown)
			for (size_t i = 0; i < renderer_parent->GetChildCount(); i++)
			{
				renderer_parent->GetChildAt(i)->Destroy();
			}

		if (model_shown) {
			for (const auto& handle : this->handle_pool)
			{
				if (!handle->Get_is_edge())
					continue;

				//MAIN SEGMENTS
				for (const auto& obj : handle->Get_all_segs())
				{
					Vector3 pos = obj->World().position.Get();
					pos -= Vector3(0, handle->Get_height_per_wall() / 2.0f, 0);

					Quaternion rot = obj->World().rotation.Get();
					Quaternion flip_rot = Quaternion::Euler(Vector3(0, 180, 0));
					rot *= flip_rot;

					int floor_index = handle->Get_floor(obj);
					int row_index = handle->Get_row(obj);

					RenderObject* newrenderer = [=]()
					{
							if (this->multisegment_models) {
								bool center3x_able = handle->Get_cur_width() >= 5 && handle->Get_cur_width() % 2 == 1;
								int center_based_x = 0;

								if (handle->Get_cur_width() % 2 == 1)
									center_based_x = row_index - (handle->Get_cur_width() / 2);
								else {
									center_based_x = row_index - (handle->Get_cur_width() / 2);

									if (center_based_x >= 0)
										center_based_x -= 1;
								}

								if (center3x_able && handle->Get_cur_height() >= 4) //3x4 walls
								{
									int choice_x = center_based_x + 1;
									choice_x = 2 - choice_x; //Flip for correct side

									if (choice_x >= 0 && choice_x < 3 && floor_index < 4)
										return new RenderObject(wall3x4_DC[floor_index][choice_x]);
								}

								if (!center3x_able || abs(center_based_x) >= 3) { //2x3 walls
									int choice_x = -1;
									int interval_x = center_based_x % 5;


									if (center_based_x > 0)
									{
										if (interval_x == 3)
											if (row_index + 1 < handle->Get_cur_width())
												choice_x = 1;
										if (interval_x == 4)
											choice_x = 0;
									}
									else {
										if (interval_x == -3)
											if (row_index - 1 >= 0)
												choice_x = 0;
										if (interval_x == -4)
											choice_x = 1;
									}


									if (choice_x >= 0 && floor_index < 3)
										return new RenderObject(wall2x3_DC[floor_index][choice_x]);
								}
							}

							if (floor_index == 0)
								return new RenderObject(wallnorm_DC[0]);
							if (floor_index > 0)
								return new RenderObject(wallnorm_DC[1]);
						
						}();

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

				//PAHABOL SEGMENTS
				for (const auto& obj : handle->Get_all_segs())
				{
					int floor_index = handle->Get_floor(obj);
					int row_index = handle->Get_row(obj);

					//iterate only the last floor
					if (handle->Get_cur_height() - 1 != floor_index) {
						continue;
					}

					Vector3 pos = obj->World().position.Get();
					pos -= Vector3(0, handle->Get_height_per_wall() / 2.0f, 0);
					pos += Vector3(0, handle->Get_height_per_wall(), 0); // add 1 more floor worth of height

					Quaternion rot = obj->World().rotation.Get();
					Quaternion flip_rot = Quaternion::Euler(Vector3(0, 180, 0));
					rot *= flip_rot;

					RenderObject* newrenderer = [=]()
						{
							return new RenderObject(roof_DC);
						}();

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

			for (const auto& roof : this->roof_pool)
			{
				for (const auto& roof_obj : roof.objs)
				{
					RenderObject* newrenderer = new RenderObject(ceiling_DC);
					newrenderer->SetParent(renderer_parent);
					newrenderer->Local().SetMatrix(roof_obj->Local().GetMatrix());
					newrenderer->SetShadowCaster();
				}
			}
		}

		for (const auto& handle : this->handle_pool)
		{
			handle->Set_visible(!model_shown);
		}
		for (const auto& roof : this->roof_pool)
		{
			roof.objs[0]->Set_enabled(!model_shown);
			roof.objs[1]->Set_enabled(!model_shown);
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
		i %= this->data.sets[s].segs.size();

		auto& seg = this->data.sets[s].segs[i].seg;
		auto handle = this->handle_pool[this->data.sets[s].segs[i].handleindex];

		auto delta_right = handle->Local().GetRight() * (handle->Local().scale.Get().x);
		auto delta_up = -Vector3(0, height * 0.5f, 0);

		l = handle->Local().position.Get() - delta_right + delta_up;
		r = handle->Local().position.Get() + delta_right + delta_up;
	}

	void BuilderBlock::ResetHandle(int s, int i) {

		auto& set = this->data.sets[s];
		auto& seg = this->data.sets[s].segs[i].seg;

		SetRoof* set_roof = nullptr;

		for (auto& roof : this->roof_pool)
		{
			if(roof.parent_index == s)
				set_roof = &roof;
		}

		const auto ResetRoof = [=](Object* roof_obj, int basis_index) {
			const auto right = data.GetPosition(this->GetHandle(s, basis_index).seg.second) + Vector3(0, height, 0);
			const auto left = data.GetPosition(this->GetHandle(s, basis_index -1).seg.first) + Vector3(0, height, 0);

			// Assuming you have your axis vectors and position vector
			Vector3 position = data.GetPosition(this->GetHandle(s, basis_index).seg.first) + Vector3(0, height, 0);
			Vector3 xAxis = right - position; // Example: local X-axis
			Vector3 yAxis = Vector3(0.0f, 0.02f, 0.0f); // Example: local Y-axis
			Vector3 zAxis = left - position; // Example: local Z-axis

			// Create the glm::mat4
			Matrix4 modelMatrix = Matrix4(1.0f); // Initialize with identity matrix

			// Assign the axis vectors to the first three columns
			modelMatrix[0] = Vector4(xAxis, 0.0f); // X-axis
			modelMatrix[1] = Vector4(yAxis, 0.0f); // Y-axis
			modelMatrix[2] = Vector4(zAxis, 0.0f); // Z-axis

			// Assign the position vector to the fourth column (translation)
			modelMatrix[3] = Vector4(position, 1.0f); // Translation

			roof_obj->Local().SetMatrix(modelMatrix);
			};
		ResetRoof(set_roof->objs[0], 0);
		ResetRoof(set_roof->objs[1], 2);

		auto handle = this->handle_pool[this->data.sets[s].segs[i].handleindex];

		Vector3 delta = data.GetPosition(seg.first) - data.GetPosition(seg.second);
		float half_mag = delta.Magnitude() * 0.5f;

		handle->Local().position.Set(Vector3::Mid(data.GetPosition(seg.first), data.GetPosition(seg.second)) + Vector3(0, height * 0.5f, 0));
		handle->Local().rotation.Set(Quaternion::LookAtRotation(delta.Cross(Vector3::Up()).Normalize(), Vector3::Up()));
		handle->Local().scale.Set(Vector3(half_mag, height * 0.5f, 0.01f));
	}

	inline bool BuilderBlock::CheckSetSegment(Vector3 p, Vector3 l, Vector3 r)
	{
		Vector3 delta_a = l - p;
		Vector3 delta_b = r - p;

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

	void BuilderBlock::InvokeUpdate(float deltatime) {
		BuilderBlockSet* moved_obj = nullptr;
		int moved_pos_l = -1;
		int moved_pos_r = -1;

		Vector3 updated_l;
		Vector3 updated_r;

		int moved_s = -1;
		int moved_i = -1;

		for (size_t s = 0; s < data.sets.size(); s++) {
			for (size_t i = 0; i < data.sets[s].segs.size(); i++)
			{
				auto& l = GetHandle(s, i);

				bool moved = this->handle_pool[l.handleindex]->CheckState(Object::ObjectStateName::TRANSFORMED_USER, this);

				if (moved) {
					moved_obj = this->handle_pool[l.handleindex];
					UpdateHandleSegment(s, i, updated_l, updated_r);
					moved_pos_l = l.seg.first;
					moved_pos_r = l.seg.second;
					moved_s = s;
					moved_i = i;
					break;
				}
			}

			if (moved_obj != nullptr)
				break;
		}

		if (moved_obj == nullptr) //nothing more to do if theres nothing that moved
			return;
		if (model_shown)
			return; //no editing if models are shown

		bool valid_move = true;

		for (size_t s = 0; s < data.sets.size(); s++) {
			for (size_t i = 0; i < data.sets[moved_s].segs.size(); i++)
			{
				auto& l = GetHandle(s, i);

				if (handle_pool[l.handleindex] == moved_obj)
					continue;

				if (l.seg.second == moved_pos_l) {
					Vector3 other_vec = updated_r;

					if (moved_s != s) {
						other_vec = data.GetPosition(GetHandle(s, i + 1).seg.second);
					}

					valid_move = valid_move && CheckSetSegment(updated_l, other_vec, data.GetPosition(l.seg.first));
					valid_move = valid_move && CheckSetSegment(data.GetPosition(l.seg.first), updated_l, data.GetPosition(GetHandle(s, i - 1).seg.first));
				}
				if (l.seg.first == moved_pos_r) {
					Vector3 other_vec = updated_l;

					if (moved_s != s) {
						other_vec = data.GetPosition(GetHandle(s, i - 1).seg.first);
					}

					valid_move = valid_move && CheckSetSegment(updated_r, other_vec, data.GetPosition(l.seg.second));
					valid_move = valid_move && CheckSetSegment(data.GetPosition(l.seg.second), updated_r, data.GetPosition(GetHandle(s, i + 1).seg.second));
				}
			}
		}

		//Prevent flipping
		auto opp_pos = handle_pool[GetHandle(moved_s, moved_i + 2).handleindex]->Local().position.Get(); //The handle opposite of the moved handle
		Vector3 opp_dir = opp_pos - moved_obj->Local().position.Get();
		auto opp_dot = opp_dir.Dot(moved_obj->Local().GetForward());
		if (opp_dot < 0)
			valid_move = false;

		if (valid_move) {
			SetPosition(moved_pos_l, updated_l);
			SetPosition(moved_pos_r, updated_r);
		}
		else {
			ResetHandle(moved_s, moved_i);

			return;
		}

		for (size_t s = 0; s < data.sets.size(); s++) {
			for (size_t i = 0; i < data.sets[s].segs.size(); i++)
			{
				auto& l = GetHandle(s, i);

				if (handle_pool[l.handleindex] == moved_obj)
					continue;

				ResetHandle(s, i);
			}
		}
	}

	void BuilderBlock::AddBlock(int root_handle) {

		if (!handle_pool[root_handle]->Get_is_edge())
			return;

		handle_pool[root_handle]->Set_is_edge(false);

		SetSeg src_seg;

		for (size_t s = 0; s < data.sets.size(); s++)
			for (size_t i = 0; i < data.sets[s].segs.size(); i++)
			{
				if (data.sets[s].segs[i].handleindex == root_handle) {
					src_seg = data.sets[s].segs[i].seg;
				}
			}

		Vector3 seg_dir = this->data.GetPosition(src_seg.second) - this->data.GetPosition(src_seg.first);
		Vector3 seg_3rd_dir = seg_dir.Cross(Vector3::Up()).Normalize() * 4.0f;

		int poolindex = data.positions.size();
		this->data.AddPosition(this->data.GetPosition(src_seg.second) - seg_3rd_dir);
		this->data.AddPosition(this->data.GetPosition(src_seg.first) - seg_3rd_dir);

		std::vector<int> corners = {
			src_seg.second,
			src_seg.first,
			poolindex + 1,
			poolindex,
		};

		AddBlock(corners.data());
	}

	void BuilderBlock::AddBlock(int corners[4]) {

		std::vector<SetSeg> src_segments = {
			{corners[0], corners[1]},
			{corners[1], corners[2]},
			{corners[2], corners[3]},
			{corners[3], corners[0]},
		};

		auto base_center = Vector3::Mid(Vector3::Mid(data.GetPosition(corners[0]), data.GetPosition(corners[1])), Vector3::Mid(data.GetPosition(corners[2]), data.GetPosition(corners[3])));
		
		BlockSet newset;

		int starting_index = 0;

		//look for existing root handle
		for (const auto& set : this->data.sets)
			for (const auto& seg : set.segs)
			{
				if(seg.seg.second == corners[0] && seg.seg.first == corners[1]) {
					newset.segs.push_back({
						.seg = {corners[0], corners[1]},
						.handleindex = seg.handleindex
						});
					this->handle_pool[seg.handleindex]->Set_is_edge(false);
					starting_index = 1;
					break;
				}
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
			
			
			newset.segs.push_back(
				{
					.seg = src_seg,
					.handleindex = (int)(handle_pool.size() - 1)
				});

			Vector3 delta = data.GetPosition(src_seg.first) - data.GetPosition(src_seg.second);
			float half_mag = delta.Magnitude() * 0.5f;

			handle->Local().position.Set(Vector3::Mid(data.GetPosition(src_seg.first), data.GetPosition(src_seg.second)) + Vector3(0, height * 0.5f, 0));
			handle->Local().rotation.Set(Quaternion::LookAtRotation(delta.Cross(Vector3::Up()).Normalize(), Vector3::Up()));
			handle->Local().scale.Set(Vector3(half_mag, height * 0.5f, 0.01f));
		}

		//ROOF
		const auto CreateRoof = [=](int basis_index) {
			auto roof_renderer = new RenderObject(ceiling_editor_DC);
			roof_renderer->SetParent(this);

			int left_index = basis_index - 1;
			if (left_index < 0)
				left_index += 4;
			int right_index = (basis_index + 1) % 4;

			const auto right = data.GetPosition(corners[right_index]) + Vector3(0, height, 0);
			const auto left = data.GetPosition(corners[left_index]) + Vector3(0, height, 0);

			// Assuming you have your axis vectors and position vector
			Vector3 position = data.GetPosition(corners[basis_index]) + Vector3(0, height, 0);
			Vector3 xAxis = right - position; // Example: local X-axis
			Vector3 yAxis = Vector3(0.0f, 0.02f, 0.0f); // Example: local Y-axis
			Vector3 zAxis = left - position; // Example: local Z-axis

			// Create the glm::mat4
			Matrix4 modelMatrix = Matrix4(1.0f); // Initialize with identity matrix

			// Assign the axis vectors to the first three columns
			modelMatrix[0] = Vector4(xAxis, 0.0f); // X-axis
			modelMatrix[1] = Vector4(yAxis, 0.0f); // Y-axis
			modelMatrix[2] = Vector4(zAxis, 0.0f); // Z-axis

			// Assign the position vector to the fourth column (translation)
			modelMatrix[3] = Vector4(position, 1.0f); // Translation

			roof_renderer->Local().SetMatrix(modelMatrix);

			return roof_renderer;
			};

		this->data.sets.push_back(newset);

		SetRoof newset_roof = {
			.objs = {CreateRoof(0), CreateRoof(2)},
			.parent_index = (int)this->data.sets.size() - 1
		};

		this->roof_pool.push_back(newset_roof);
	}
}
