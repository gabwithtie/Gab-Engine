#include "BuilderBlock.h"

#include <vector>

#include "BuilderBlockFace.h"
#include "Graphics/gbe_graphics.h"
#include "Asset/gbe_asset.h"
#include "Math/gbe_math.h"

#include "AnitoBuilderExtension.h"

namespace gbe::ext::AnitoBuilder {
	void BuilderBlock::LoadAssets()
	{
		//OBJECT SETUP
		ceiling_parent = new Object();
		ceiling_parent->PushEditorFlag(Object::EditorFlags::STATIC_POS_X);
		ceiling_parent->PushEditorFlag(Object::EditorFlags::STATIC_POS_Z);
		ceiling_parent->PushEditorFlag(Object::EditorFlags::STATIC_ROT);
		ceiling_parent->PushEditorFlag(Object::EditorFlags::STATIC_SCALE);
		ceiling_parent->SetParent(this);
		ceiling_parent->Local().position.Set(Vector3(0, this->height, 0));

		//MATERIAL SETUP
		auto material = asset::Material::GetAssetById("plaster");

		//Editor
		ceiling_editor_DC = RenderPipeline::RegisterDrawCall(asset::Mesh::GetAssetById("horizontal_axis_triangle"), asset::Material::GetAssetById("grid"));
		
		//Main - normal
		ceiling_DC = RenderPipeline::RegisterDrawCall(asset::Mesh::GetAssetById("horizontal_axis_triangle"), material);
		ceiling_1_DC = RenderPipeline::RegisterDrawCall(asset::Mesh::GetAssetById("axisroof"), material);
		
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

		windowwall_DC[0][0] = RenderPipeline::RegisterDrawCall(asset::Mesh::GetAssetById("windowwall_1-1"), material);
		windowwall_DC[0][1] = RenderPipeline::RegisterDrawCall(asset::Mesh::GetAssetById("windowwall_1-2"), material);
	}

	BuilderBlock::BuilderBlock(gbe::Vector3 corners[4], float height)
		: Object(), Update()
	{
		this->SetName("Anito Builder Block");
		this->height = height;

		LoadAssets();

		//OBJECTS
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

		GeneralInit();
		
		Refresh();
	}


	void BuilderBlock::GeneralInit()
	{
		Object::GeneralInit();

		this->PushEditorFlag(Object::EditorFlags::SERIALIZABLE);

		//INSPECTOR
		{
			auto field = new gbe::editor::InspectorFloat();
			field->name = "segment_max_width";
			field->getter = [=]() { return this->wall_max_width; };
			field->setter = [=](float val) {
				this->wall_max_width = val;
				this->Refresh();
				};

			this->inspectorData->fields.push_back(field);
		}

		{
			auto field = new gbe::editor::InspectorFloat();
			field->name = "segment_max_height";
			field->getter = [=]() { return this->wall_max_height; };
			field->setter = [=](float val) {
				this->wall_max_height = val;
				this->Refresh();
				};

			this->inspectorData->fields.push_back(field);
		}

		{
			auto field = new gbe::editor::InspectorFloat();
			field->name = "segment_depth";
			field->getter = [=]() { return this->thickness; };
			field->setter = [=](float val) {
				this->thickness = val;
				this->Refresh();
				};

			this->inspectorData->fields.push_back(field);
		}
	}

	SerializedObject BuilderBlock::Serialize()
	{
		auto obj = Object::Serialize();

		for (size_t s = 0; s < data.sets.size(); s++) {
			for (size_t i = 0; i < data.sets[s].segs.size(); i++)
			{
				auto handle = this->handle_pool[this->data.sets[s].segs[i].handleindex];
			}
		}

		auto data = gbe::asset::serialization::gbeParser::ExportClassStr(this->data);
		obj.serialized_variables.insert_or_assign("data", data);
		obj.serialized_variables.insert_or_assign("height", std::to_string(this->height));

		return obj;
	}

	BuilderBlock::BuilderBlock(SerializedObject* data) : Object(data)
	{
		this->SetName("Anito Builder Block");
		BuilderBlockData newdata;
		gbe::asset::serialization::gbeParser::PopulateClassStr(newdata, data->serialized_variables["data"]);
		this->height = std::stof(data->serialized_variables["height"]);
		
		LoadAssets();


		for (const auto& pos : newdata.positions)
		{
			this->data.AddPosition({ pos[0], pos[1] , pos[2] });
		}

		//ADDING
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

		//OVERRWRITING
		this->data = newdata;

		GeneralInit();
		
		Refresh();
	}

	void BuilderBlock::UpdateModelShown()
	{
		//set editor handle visibility
		for (const auto& handle : this->handle_pool)
		{
			handle->Set_visible(!model_shown);
		}
		//set editor roof handle visibility
		for (const auto& roofset : this->roof_pool)
		{
			for (const auto& roofobj : roofset.handle_renderers)
			{
				roofobj->Set_enabled(!model_shown);
			}
		}

		std::cout << "[ANITOBUILDERBLOCK]: Updating model visibility." << std::endl;

		if (!model_shown) {
			for (const auto& renderer : display_renderers)
			{
				renderer->Destroy();
			}

			display_renderers.clear();
		}

		if (model_shown) {
			auto temppool = std::vector<BuilderBlockFace*>(this->handle_pool);
			
			float inset_distance = 2.0f;
			float roofheight = 2.5;
			float roof_overshoot = 1.5;

			const auto GetQuadInsetPoints = [=](BlockSet& targetSet)
				{
					std::vector<std::pair<Vector3, Vector3>> allInsetSegments;
					int numCorners = targetSet.segs.size();

					enum VertexType
					{
						BISECTOR,
						L_EDGE,
						R_EDGE
					};

					auto CalculatePointInset = [&](int i, VertexType returntype) -> Vector3
						{
							int idxPrev = (i + numCorners - 1) % numCorners;
							int idxNext = i;

							bool prevIsEdge = handle_pool[targetSet.segs[idxPrev].handleindex]->Get_is_edge();
							bool nextIsEdge = handle_pool[targetSet.segs[idxNext].handleindex]->Get_is_edge();

							Vector3 pMid = data.GetPosition(targetSet.segs[i].seg.first);
							Vector3 pPrev = data.GetPosition(targetSet.segs[idxPrev].seg.first);
							Vector3 pNext = data.GetPosition(targetSet.segs[idxNext].seg.second);

							Vector3 dPrev = ((Vector3)(pPrev - pMid)).Normalize();
							Vector3 dNext = ((Vector3)(pNext - pMid)).Normalize();

							// CASE 1: True Exterior Corner (Inward Bisector)
							if (returntype == BISECTOR) {
								Vector3 bisector = ((Vector3)(dPrev + dNext)).Normalize();
								float dot = std::clamp(dPrev.Dot(dNext), -1.0f, 1.0f);
								float angle = acos(dot);
								float len = inset_distance / sin(angle * 0.5f);
								if (dNext.Cross(dPrev).y < 0) bisector = -bisector;
								return pMid + (bisector * len);
							}
							else {
								float dot = std::clamp(dPrev.Dot(dNext), -1.0f, 1.0f);
								float angle = acos(dot);
								float slideLen = inset_distance / sin(angle);

								// We slide away from the junction point toward the edge
								return (returntype == L_EDGE) ? pMid + (dPrev * slideLen) : pMid + (dNext * slideLen);
							}

							return pMid;
						};

					for (int i = 0; i < numCorners; i++)
					{
						bool currentIsEdge = handle_pool[targetSet.segs[i].handleindex]->Get_is_edge();

						Vector3 L_bisector = CalculatePointInset(i, BISECTOR);
						Vector3 L_edge = CalculatePointInset(i, R_EDGE);
						Vector3 R_bisector = CalculatePointInset((i + 1) % numCorners, BISECTOR);
						Vector3 R_edge = CalculatePointInset((i + 1) % numCorners, L_EDGE);

						if (currentIsEdge) {
							// Normal Edge: These close inwards as usual
							allInsetSegments.push_back({ L_bisector, R_bisector });
						}
						else {
							allInsetSegments.push_back({ L_bisector, L_edge });
							allInsetSegments.push_back({ R_edge, R_bisector });
						}
					}

					return allInsetSegments;
				};

			std::vector<std::pair<Vector3, Vector3>> insetpolygon;

			for (auto& set : this->data.sets)
			{
				auto insetpoints = GetQuadInsetPoints(set);

				for (const auto& i_pair : insetpoints)
				{
					insetpolygon.push_back(i_pair);

					auto handle = new BuilderBlockFace(this, 0);
					handle->SetParent(this);
					display_renderers.push_back(handle);

					handle->Set_visible(false);

					temppool.push_back(handle);

					auto a = i_pair.first;
					auto b = i_pair.second;

					a.y = height;
					b.y = height + roofheight;
					handle->SetPositions(a, b);
				}
			}

			for (auto& handle : temppool)
			{
				if (!handle->Get_is_edge())
					continue;

				auto handle_data = GetSeg(handle);

				bool odd = handle->Get_cur_width() % 2 == 1;
				bool can_put_facade = handle->Get_cur_height() >= 3 && handle->Get_cur_width() >= 5 && odd;

				const auto get_center_x = [=](RenderObject* obj) {
					int row_index = handle->Get_row(obj);

					int center_based_x = 0;

					if (handle->Get_cur_width() % 2 == 1)
						center_based_x = -(row_index - (handle->Get_cur_width() / 2));
					else {
						center_based_x = -(row_index - (handle->Get_cur_width() / 2));
					}

					return -center_based_x;
					};


				// MAIN SEGMENTS
				for (const auto& obj : handle->Get_all_segs())
				{
					int floor_index = handle->Get_floor(obj);

					Vector3 pos = obj->World().position.Get();
					pos -= Vector3(0, handle->Get_height_per_wall() / 2.0f, 0);

					// ----------------------------

					const auto process_renderobject = [&](RenderObject* newrenderer) {
						newrenderer->SetParent(handle);
						newrenderer->PushEditorFlag(Object::EditorFlags::SELECT_PARENT_INSTEAD);
						display_renderers.push_back(newrenderer);

						auto inv__import_scale = Vector3(1.0 / wall_import_width, 1.0f / wall_import_height_from_zero, 1);
						Vector3 final_scale = Vector3(1);
						auto target_local_scale = obj->Local().scale.Get();
						final_scale.x = (inv__import_scale.x * target_local_scale.x);
						final_scale.y = (inv__import_scale.y * target_local_scale.y);
						final_scale.z = thickness;

						newrenderer->Local().scale.Set(final_scale);
						newrenderer->World().position.Set(pos);

						return newrenderer;
						};

					int row_index = handle->Get_row(obj);
					int center_based_x = get_center_x(obj);

					process_renderobject([&]()
						{
							if (handle_data != nullptr) {
								if (handle_data->center_facade_type > 0) {
									if (can_put_facade) {
										int choice_x = center_based_x + 1;
										if (choice_x >= 0 && choice_x < 3 && floor_index < 4) {
											if (handle_data->center_facade_type == 2) {
												if (floor_index == 0 || floor_index == 3) return new RenderObject(windowwall_DC[0][0]);
												else if (floor_index < 4) return new RenderObject(windowwall_DC[0][1]);
											}
											return new RenderObject(wall3x4_DC[floor_index][choice_x]);
										}
										if (choice_x == -1 || choice_x == 3) {
											if (floor_index == 0 || floor_index == 3) return new RenderObject(windowwall_DC[0][0]);
											else if (floor_index < 4) return new RenderObject(windowwall_DC[0][1]);
										}
									}
									if (handle_data->edge_designs_interval > 0)
										if (handle->Get_cur_height() >= 3) {
											int choice_x = -1;
											int interval_x = abs(center_based_x) % 4;
											if (center_based_x > 0) {
												if (interval_x == 1) choice_x = 0;
												if (interval_x == 2) choice_x = 1;
											}
											else {
												if (interval_x == 1) choice_x = 1;
												if (interval_x == 2) choice_x = 0;
											}
											if (interval_x == 1 && abs(center_based_x) + 2 > (handle->Get_cur_width() / 2)) choice_x = -1;
											if (interval_x == 2 && abs(center_based_x) + 1 > (handle->Get_cur_width() / 2)) choice_x = -1;
											if (choice_x >= 0 && floor_index < 3) return new RenderObject(wall2x3_DC[floor_index][choice_x]);
										}
								}
								if (floor_index == 0) return new RenderObject(wallnorm_DC[0]);
								if (floor_index > 0)
								{
									return new RenderObject(wallnorm_DC[1]);
								}
							}

							return new RenderObject(wallnorm_DC[1]);
						}());

					//iterate only the last floor
					if (handle->Get_cur_height() - 1 != floor_index) {
						continue;
					}

					pos += Vector3(0, handle->Get_height_per_wall(), 0); // add 1 more floor worth of height

					if (handle_data != nullptr) {
						process_renderobject([&]()
							{
								if (handle_data->center_facade_type > 0) {
									if (can_put_facade && handle->Get_cur_height() == 3) //3x4 walls, minus 1 because the last layer can be pahabol
									{
										int choice_x = center_based_x + 1;

										if (choice_x >= 0 && choice_x < 3 && floor_index < 4) {
											if (handle_data->center_facade_type == 2)
												return new RenderObject(windowwall_DC[0][1]);
											else
												return new RenderObject(wall3x4_DC[3][choice_x]);
										}

										if (choice_x == -1 || choice_x == 3) {
											return new RenderObject(windowwall_DC[0][0]);
										}
									}
								}

								return new RenderObject(roof_DC);
							}());
					}
				}
			}

			for (const auto& roof : this->roof_pool)
			{
				const auto createroofobj = [=](DrawCall* dc, int index, float height, float offset, float inset) {
					RenderObject* base_ceiling = new RenderObject(dc);
					base_ceiling->SetParent(ceiling_parent);
					base_ceiling->PushEditorFlag(Object::EditorFlags::SELECT_PARENT_INSTEAD);
					display_renderers.push_back(base_ceiling);

					ResetRoof(base_ceiling, roof.parent_index, index, height, inset, offset);
					};

				createroofobj(ceiling_DC, 0, 0.02f, 0, 0);
				createroofobj(ceiling_DC, 2, 0.02f, 0, 0);
				createroofobj(ceiling_1_DC, 0, 0.5f, roofheight, inset_distance - 0.5f);
				createroofobj(ceiling_1_DC, 2, 0.5f, roofheight, inset_distance - 0.5f);
			}
		}
	}

	void BuilderBlock::UpdateHandleSegment(int s, int i, Vector3& l, Vector3& r)
	{
		i %= this->data.sets[s].segs.size();

		auto& seg = this->data.sets[s].segs[i].seg;
		auto handle = this->handle_pool[this->data.sets[s].segs[i].handleindex];

		auto delta_right = handle->Local().GetRight() * (handle->Local().scale.Get().x);

		l = handle->Local().position.Get() + delta_right;
		r = handle->Local().position.Get() - delta_right;
	}

	void BuilderBlock::Refresh()
	{
		SetModelShown(false);
		ResetAllHandles();
		SetModelShown(true);
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

		if (set_roof == nullptr)
			return;

		ResetRoof(set_roof->handle_renderers[0], s, 0);
		ResetRoof(set_roof->handle_renderers[1], s, 2);

		auto handle = this->handle_pool[this->data.sets[s].segs[i].handleindex];
	
		auto second = data.GetPosition(seg.second);
		second.y += height;
		handle->SetPositions(data.GetPosition(seg.first), second);
	}

	void BuilderBlock::ResetAllHandles()
	{
		for (size_t s = 0; s < data.sets.size(); s++) {
			for (size_t i = 0; i < data.sets[s].segs.size(); i++)
			{
				ResetHandle(s, i);
			}
		}
	}

	bool BuilderBlock::CheckSetSegment(Vector3 p, Vector3 l, Vector3 r)
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

	void BuilderBlock::ResetRoof(Object* roof, int s, int i, float _height, float _inset, float _y)
	{
		auto right = data.GetPosition(this->GetHandle(s, i).seg.second) + Vector3(0, _y, 0);
		auto left = data.GetPosition(this->GetHandle(s, i - 1).seg.first) + Vector3(0, _y, 0);

		// Assuming you have your axis vectors and position vector
		Vector3 position = data.GetPosition(this->GetHandle(s, i).seg.first) + Vector3(0, _y, 0);

		Vector3 xAxis = right - position; // Example: local X-axis
		Vector3 yAxis = Vector3(0.0f, _height, 0.0f); // Example: local Y-axis
		Vector3 zAxis = left - position; // Example: local Z-axis

		auto r_norm = xAxis.Normalize();
		auto l_norm = zAxis.Normalize();

		position += (r_norm + l_norm) * _inset;
		xAxis -= r_norm * 2.0f * _inset;
		zAxis -= l_norm * 2.0f * _inset;

		// Create the glm::mat4
		Matrix4 modelMatrix = Matrix4(1.0f); // Initialize with identity matrix

		// Assign the axis vectors to the first three columns
		modelMatrix[0] = Vector4(xAxis, 0.0f); // X-axis
		modelMatrix[1] = Vector4(yAxis, 0.0f); // Y-axis
		modelMatrix[2] = Vector4(zAxis, 0.0f); // Z-axis

		// Assign the position vector to the fourth column (translation)
		modelMatrix[3] = Vector4(position, 1.0f); // Translation

		roof->Local().SetMatrix(modelMatrix);
	}

	void BuilderBlock::InvokeUpdate(float deltatime) {

		//ROOF MOVING
		bool roof_moved = this->ceiling_parent->CheckState(Object::ObjectStateName::TRANSFORMED_USER, this);
		if (roof_moved) {
			this->height = this->ceiling_parent->Local().position.Get().y;
			SetModelShown(false);
			ResetAllHandles();
			return;
		}

		//SEGMENT MOVING
		bool moved_any = false;
		bool valid_move = true; //querry validity across all possible moved handles

		std::vector<std::pair<int, int>> pos_resets; //queery ResetHandle calls
		std::vector<std::pair<int, Vector3>> pos_commits; // querry SetPosition calls

		for (size_t s = 0; s < data.sets.size(); s++) {
			for (size_t i = 0; i < data.sets[s].segs.size(); i++)
			{
				auto& l = GetHandle(s, i);

				bool moved = this->handle_pool[l.handleindex]->CheckState(Object::ObjectStateName::TRANSFORMED_USER, this);

				if (!moved)
					continue;

				Vector3 updated_l;
				Vector3 updated_r;

				BuilderBlockFace* moved_obj = this->handle_pool[l.handleindex];
				UpdateHandleSegment(s, i, updated_l, updated_r);
				int moved_pos_l = l.seg.first;
				int moved_pos_r = l.seg.second;
				int moved_s = s;
				int moved_i = i;
				
				for (size_t s = 0; s < data.sets.size(); s++) {
					for (size_t i = 0; i < data.sets[moved_s].segs.size(); i++)
					{
						auto& handle = GetHandle(s, i);
						
						if (handle_pool[handle.handleindex] == moved_obj)
							continue;

						if (handle.seg.second == moved_pos_l) {
							Vector3 other_vec = updated_r;

							if (moved_s != s) {
								other_vec = data.GetPosition(GetHandle(s, i + 1).seg.second);
							}

							valid_move = valid_move && CheckSetSegment(updated_l, other_vec, data.GetPosition(handle.seg.first));
							valid_move = valid_move && CheckSetSegment(data.GetPosition(handle.seg.first), updated_l, data.GetPosition(GetHandle(s, i - 1).seg.first));
						}
						if (handle.seg.first == moved_pos_r) {
							Vector3 other_vec = updated_l;

							if (moved_s != s) {
								other_vec = data.GetPosition(GetHandle(s, i - 1).seg.first);
							}

							valid_move = valid_move && CheckSetSegment(updated_r, other_vec, data.GetPosition(handle.seg.second));
							valid_move = valid_move && CheckSetSegment(data.GetPosition(handle.seg.second), updated_r, data.GetPosition(GetHandle(s, i + 1).seg.second));
						}
					}
				}

				//Prevent flipping
				auto opp_pos = handle_pool[GetHandle(moved_s, moved_i + 2).handleindex]->Local().position.Get(); //The handle opposite of the moved handle
				Vector3 opp_dir = opp_pos - moved_obj->Local().position.Get();
				auto opp_dot = opp_dir.Dot(-moved_obj->Local().GetForward());
				if (opp_dot < 0)
					valid_move = false;

				if (valid_move) {
					moved_any = true;

					pos_commits.push_back({ moved_pos_l, updated_l });
					pos_commits.push_back({ moved_pos_r, updated_r });
				}
				else {
					pos_resets.push_back({ moved_s, moved_i });
					break;
				}
			}
		}

		if (valid_move && moved_any) {
			
			if (model_shown) // toggle model off when a handle is moved
				SetModelShown(false);

			for (const auto& commit : pos_commits)
			{
				SetPosition(commit.first, commit.second);
			}

			//update graphical handles
			for (size_t s = 0; s < data.sets.size(); s++) {
				for (size_t i = 0; i < data.sets[s].segs.size(); i++)
				{
					auto& l = GetHandle(s, i);
					ResetHandle(s, i);
				}
			}
		}

		if(!valid_move) {
			for (const auto& reset : pos_resets)
			{
				ResetHandle(reset.first, reset.second);
			}
		}

		if (!roof_moved && !moved_any) //toggle model back once nothing has moved
		{
			if (!model_shown)
				SetModelShown(true);

			return; //nothing more to do
		}
	}

	BlockSeg* BuilderBlock::GetSeg(int handle_i)
	{
		if (handle_i >= 0) {
			for (size_t set_i = 0; set_i < this->data.sets.size(); set_i++)
			{
				const auto& set = this->data.sets[set_i];

				for (size_t seg_i = 0; seg_i < set.segs.size(); seg_i++)
				{
					const auto& seg = set.segs[seg_i];

					if (seg.handleindex == handle_i) {
						return &this->data.sets[set_i].segs[seg_i];
					}
				}
			}
		}

		return nullptr;
	}

	BlockSeg* BuilderBlock::GetSeg(BuilderBlockFace* face)
	{
		auto handle_i = -1;

		for (size_t i = 0; i < this->handle_pool.size(); i++)
		{
			if (handle_pool[i] == face) {
				handle_i = i;
				break;
			}
		}

		return GetSeg(handle_i);
	}

	void BuilderBlock::AddBlock(int root_handle) {

		if (!handle_pool[root_handle]->Get_is_edge())
			return;

		handle_pool[root_handle]->Set_is_edge(false);

		PosPair src_seg;

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

		std::vector<PosPair> src_segments = {
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

		int incr = 0;
		for (size_t i = starting_index; i < src_segments.size(); i++)
		{
			const auto& src_seg = src_segments[i];

			newset.segs.push_back(
				{
					.seg = src_seg,
					.handleindex = (int)handle_pool.size() + incr
				});

			incr++;
		}
		this->data.sets.push_back(newset);
		for (size_t i = starting_index; i < src_segments.size(); i++)
		{
			auto handle = new BuilderBlockFace(this, handle_pool.size());

			handle_pool.push_back(handle);
			handle->SetParent(this);

			handle->PushEditorFlag(Object::EditorFlags::STATIC_POS_Y);
			handle->PushEditorFlag(Object::EditorFlags::STATIC_ROT_X);
			handle->PushEditorFlag(Object::EditorFlags::STATIC_ROT_Z);
			handle->PushEditorFlag(Object::EditorFlags::STATIC_SCALE_Y);
			handle->PushEditorFlag(Object::EditorFlags::STATIC_SCALE_Z);
			handle->PushEditorFlag(Object::EditorFlags::NON_DIRECT_EDITABLE);
		}

		//ROOF
		const auto CreateRoof = [=](int basis_index) {
			auto roof_renderer = new RenderObject(ceiling_editor_DC);
			roof_renderer->SetParent(ceiling_parent);

			return roof_renderer;
			};


		SetRoof newset_roof = {
			.handle_renderers = {CreateRoof(0), CreateRoof(2)},
			.parent_index = (int)this->data.sets.size() - 1
		};

		this->roof_pool.push_back(newset_roof);

		Refresh();
	}
}
