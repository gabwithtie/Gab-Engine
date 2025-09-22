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
			root_block->AddBlock(this);
			};

		this->inspectorData->fields.push_back(add_block_button);
	}

	void BuilderBlockSet::OnLocalTransformationChange(TransformChangeType type) {
		Object::OnLocalTransformationChange(type);
		
		if (type != TransformChangeType::SCALE) {
			return;
		}

		float width = this->Local().scale.Get().x * 2;
		int walls_needed = ceil(width / root_block->wall_max_width);
		float shrink_factor = width / ((float)walls_needed * root_block->wall_max_width);
		float final_wall_width = shrink_factor * root_block->wall_max_width;
		float local_wall_scale_x = (final_wall_width / 2) / this->Local().scale.Get().x;

		Vector3 axis = this->Local().GetRight();
		Vector3 start_pos = this->World().position.Get();
		start_pos -= axis * (width / 2);
		start_pos += axis * (final_wall_width / 2);
		Vector3 end_pos = start_pos;
		end_pos += axis * final_wall_width * (float)walls_needed;

		if (walls_needed < this->renderObjects.size()) {
			int seg_count = this->renderObjects.size();
			for (size_t i = 0; i < seg_count; i++)
			{
				this->renderObjects[i]->Destroy();
			}
			this->renderObjects.clear();
		}

		if (walls_needed > this->renderObjects.size()) {
			int need_to_create = walls_needed - this->renderObjects.size();
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
			if (i > walls_needed) {
				this->renderObjects[i]->Destroy();
				continue;
			}

			float t = (float)i / (float)walls_needed;
			Vector3 pos = Vector3::Lerp(start_pos, end_pos, t);

			this->renderObjects[i]->World().position.Set(pos);
			this->renderObjects[i]->Local().scale.Set(Vector3(local_wall_scale_x, 1, 1));
		}
	}
}