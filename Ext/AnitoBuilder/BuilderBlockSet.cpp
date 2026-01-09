#include "BuilderBlockSet.h"
#include "BuilderBlock.h"

namespace gbe::ext::AnitoBuilder {
	BuilderBlockSet::BuilderBlockSet(BuilderBlock* root_block):
		type1_renderers(*this)
	{
		this->root_block = root_block;

		this->type1_renderers.drawcall = RenderPipeline::RegisterDrawCall(asset::Mesh::GetAssetById("cube"), asset::Material::GetAssetById("preview_wall_1"));
		
		//EDITOR OBJECTS
		handle_ro = new RigidObject(true);
		handle_ro->SetParent(this);
		auto collider = new BoxCollider();
		collider->SetParent(handle_ro);
		handle_ro->PushEditorFlag(Object::EditorFlags::SELECT_PARENT_INSTEAD);

		//INSPECTOR
		this->SetName("Anito Builder Block Set");

		{
			auto field = new gbe::editor::InspectorBool();
			field->name = "allow special walls";
			field->x = &this->allow_special_walls;

			this->inspectorData->fields.push_back(field);
		}
		{
			auto field = new gbe::editor::InspectorBool();
			field->name = "is backside";
			field->x = &this->is_backside;

			this->inspectorData->fields.push_back(field);
		}

		auto add_block_button = new gbe::editor::InspectorButton();
		add_block_button->name = "Append Block";
		add_block_button->onpress = [=]() {
			if(this->is_edge)
				root_block->AddBlock(this);
			};

		this->inspectorData->fields.push_back(add_block_button);
	}

	void BuilderBlockSet::OnLocalTransformationChange(TransformChangeType type) {
		Object::OnLocalTransformationChange(type);

		//LAYER VARS
		float height = handle_ro->Local().scale.Get().y * 2;
		int layers_needed = ceil(height / root_block->wall_max_height);
		float shrink_factor_y = height / ((float)layers_needed * root_block->wall_max_height);
		float final_wall_height = shrink_factor_y * root_block->wall_max_height;
		this->height_per_wall = final_wall_height;
		float local_wall_scale_y = (final_wall_height / 2) / handle_ro->Local().scale.Get().y;
		
		float start_pos_y = this->World().position.Get().y;
		start_pos_y -= height / 2;
		start_pos_y += final_wall_height / 2;
		float end_pos_y = start_pos_y;
		end_pos_y += final_wall_height * (float)layers_needed;
		
		//WALL VARS
		float width = handle_ro->Local().scale.Get().x * 2;
		int walls_needed = ceil(width / root_block->wall_max_width);
		float shrink_factor_x = width / ((float)walls_needed * root_block->wall_max_width);
		float final_wall_width = shrink_factor_x * root_block->wall_max_width;
		this->width_per_wall = final_wall_width;
		float local_wall_scale_x = (final_wall_width / 2) / handle_ro->Local().scale.Get().x;
		
		Vector3 axis = this->Local().GetRight();
		Vector3 start_pos_x = this->World().position.Get();
		start_pos_x -= axis * (width / 2);
		start_pos_x += axis * (final_wall_width / 2);
		Vector3 end_pos_x = start_pos_x;
		end_pos_x += axis * final_wall_width * (float)walls_needed;

		int total_segs_needed = walls_needed * layers_needed;

		if (total_segs_needed != this->renderObjects.size()) {
			int seg_count = this->renderObjects.size();
			for (size_t i = 0; i < seg_count; i++)
			{
				this->renderObjects[i]->Destroy();
			}
			this->renderObjects.clear();

			type1_renderers.ResetPool();
		}

		int seg_count = this->renderObjects.size();
		
		type1_renderers.ResetGetIndex();
		for (size_t i = 0; i < total_segs_needed; i++)
		{
			int x = i % walls_needed;
			int y = floor(i / walls_needed);

			float t_x = (float)x / (float)walls_needed;
			Vector3 pos = Vector3::Lerp(start_pos_x, end_pos_x, t_x);

			float t_y = (float)y / (float)layers_needed;
			float pos_y = glm::mix(start_pos_y, end_pos_y, t_y);

			pos.y = pos_y;

			//Create or get here
			RenderObject* renderertomove = type1_renderers.Get();

			this->renderObjects[i]->World().position.Set(pos);
			this->renderObjects[i]->Local().scale.Set(Vector3(local_wall_scale_x, local_wall_scale_y, 1));

			object_floors[this->renderObjects[i]] = y;
			object_rows[this->renderObjects[i]] = x;
		}

		this->cur_width = walls_needed;
		this->cur_height = layers_needed;
	}
}