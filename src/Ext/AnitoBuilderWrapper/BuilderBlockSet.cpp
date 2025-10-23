#include "BuilderBlockSet.h"
#include "BuilderBlock.h"

namespace gbe::ext::AnitoBuilder {
	BuilderBlockSet::BuilderBlockSet(BuilderBlock* root_block)
	{
		this->root_block = root_block;

		//EDITOR OBJECTS
		handle_ro = new RigidObject(true);
		handle_ro->SetParent(this);
		auto collider = new BoxCollider();
		collider->SetParent(handle_ro);
		handle_ro->PushEditorFlag(Object::EditorFlags::SELECT_PARENT_INSTEAD);

		//INSPECTOR
		this->SetName("Anito Builder Block Set");

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
		float height = this->Local().scale.Get().y * 2;
		int layers_needed = ceil(height / root_block->wall_max_height);
		float shrink_factor_y = height / ((float)layers_needed * root_block->wall_max_height);
		float final_wall_height = shrink_factor_y * root_block->wall_max_height;
		this->height_per_wall = final_wall_height;
		float local_wall_scale_y = (final_wall_height / 2) / this->Local().scale.Get().y;
		
		float start_pos_y = this->World().position.Get().y;
		start_pos_y -= height / 2;
		start_pos_y += final_wall_height / 2;
		float end_pos_y = start_pos_y;
		end_pos_y += final_wall_height * (float)layers_needed;
		
		//WALL VARS
		float width = this->Local().scale.Get().x * 2;
		int walls_needed = ceil(width / root_block->wall_max_width);
		float shrink_factor_x = width / ((float)walls_needed * root_block->wall_max_width);
		float final_wall_width = shrink_factor_x * root_block->wall_max_width;
		this->width_per_wall = final_wall_width;
		float local_wall_scale_x = (final_wall_width / 2) / this->Local().scale.Get().x;
		
		Vector3 axis = this->Local().GetRight();
		Vector3 start_pos_x = this->World().position.Get();
		start_pos_x -= axis * (width / 2);
		start_pos_x += axis * (final_wall_width / 2);
		Vector3 end_pos_x = start_pos_x;
		end_pos_x += axis * final_wall_width * (float)walls_needed;

		int total_segs_needed = walls_needed * layers_needed;

		if (total_segs_needed < this->renderObjects.size()) {
			int seg_count = this->renderObjects.size();
			for (size_t i = 0; i < seg_count; i++)
			{
				this->renderObjects[i]->Destroy();
			}
			this->renderObjects.clear();
		}

		if (total_segs_needed > this->renderObjects.size()) {
			int need_to_create = total_segs_needed - this->renderObjects.size();
			for (size_t i = 0; i < need_to_create; i++)
			{
				auto renderer = new RenderObject(RenderObject::cube);
				renderer->SetParent(handle_ro);
				this->renderObjects.push_back(renderer);
			}
		}

		int seg_count = this->renderObjects.size();
		for (size_t i = 0; i < seg_count; i++)
		{
			if (i > total_segs_needed) {
				this->renderObjects[i]->Destroy();
				continue;
			}

			int x = i % walls_needed;
			int y = floor(i / walls_needed);

			float t_x = (float)x / (float)walls_needed;
			Vector3 pos = Vector3::Lerp(start_pos_x, end_pos_x, t_x);

			float t_y = (float)y / (float)layers_needed;
			float pos_y = glm::mix(start_pos_y, end_pos_y, t_y);

			pos.y = pos_y;

			this->renderObjects[i]->World().position.Set(pos);
			this->renderObjects[i]->Local().scale.Set(Vector3(local_wall_scale_x, local_wall_scale_y, 1));

			object_floors[this->renderObjects[i]] = y;
			object_rows[this->renderObjects[i]] = x;
		}

		this->cur_width = walls_needed;
		this->cur_height = layers_needed;
	}
}