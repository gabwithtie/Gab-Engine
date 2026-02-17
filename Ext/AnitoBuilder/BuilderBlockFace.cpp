#include "BuilderBlockFace.h"
#include "BuilderBlock.h"

namespace gbe::ext::AnitoBuilder {
	BuilderBlockFace::BuilderBlockFace(BuilderBlock* root_block, int index):
		type1_renderers(*this)
	{
		this->root_block = root_block;

		this->type1_renderers.drawcall = RenderPipeline::RegisterDrawCall(asset::Mesh::GetAssetById("cube"), asset::Material::GetAssetById("preview_wall_1"));
		
		//INSPECTOR
		this->SetName("Anito Builder Block Set");

		{
			static std::vector<std::string> labels = { "none", "front", "back", "pillars"};

			auto field = new gbe::editor::InspectorChoice();
			field->labels = &labels;
			field->getter = [=]() {
				auto segdata = this->root_block->GetSeg(this);

				if (segdata != nullptr)
					return segdata->decoration_type;

				return 0;
				};
			field->setter = [=](int val) {
				auto segdata = this->root_block->GetSeg(this);
				if (segdata != nullptr)
					segdata->decoration_type = val;

				this->root_block->Refresh();
				};
			field->name = "decoration_type";

			this->inspectorData->fields.push_back(field);
		}
		{
			auto field = new gbe::editor::InspectorFloat();
			field->getter = [=]() {
				auto segdata = this->root_block->GetSeg(this);

				if (segdata != nullptr)
					return segdata->edge_designs_interval;

				return 0;
				};
			field->setter = [=](float val) {
				auto segdata = this->root_block->GetSeg(this);
				if (segdata != nullptr)
					segdata->edge_designs_interval = val;
				this->root_block->Refresh();
				};
			field->name = "edge_designs_interval";

			this->inspectorData->fields.push_back(field);
		}
		{
			auto field = new gbe::editor::InspectorFloat();
			field->getter = [=]() {
				auto segdata = this->root_block->GetSeg(this);

				if (segdata != nullptr)
					return segdata->edge_designs_offset;

				return 0;
				};
			field->setter = [=](float val) {
				auto segdata = this->root_block->GetSeg(this);
				if (segdata != nullptr)
					segdata->edge_designs_offset = val;
				this->root_block->Refresh();
				};
			field->name = "edge_designs_offset";

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

	void BuilderBlockFace::SetPositions(Vector3 local_a, Vector3 local_b)
	{
		Vector3 delta = local_b - local_a;
		delta.y = 0;

		float half_mag = delta.Magnitude() * 0.5f;

		auto pos = Vector3::Mid(local_a, local_b);
		pos.y = local_a.y;

		this->Local().position.Set(pos);
		this->Local().rotation.Set(Quaternion::LookAtRotation(delta.Cross(Vector3::Up()).Normalize(), Vector3::Up()));
		this->Local().scale.Set(Vector3(half_mag, local_b.y - local_a.y, 1.0f));
	}

	void BuilderBlockFace::OnLocalTransformationChange(TransformChangeType type) {
		Object::OnLocalTransformationChange(type);

		// --- LAYER VARS (Local Y: 0 to 1) ---
		float height = this->Local().scale.Get().y;
		int layers_needed = ceil(height / root_block->wall_max_height);
		this->height_per_wall = height / (float)layers_needed; // Actual height in world units

		// Scale relative to parent (1.0 / layers_needed if pivot is at base)
		float local_wall_scale_y = 1.0f / (float)layers_needed;

		// Local Y range
		float local_start_y = 0.0f + (local_wall_scale_y * 0.5f); // Half-offset to center the segment
		float local_end_y = 1.0f - (local_wall_scale_y * 0.5f);

		// --- WALL VARS (Local X: -1 to 1) ---
		float width = this->Local().scale.Get().x * 2;
		int walls_needed = ceil(width / root_block->wall_max_width);
		this->width_per_wall = width / (float)walls_needed;

		// Scale relative to parent (-1 to 1 is a range of 2.0)
		float local_wall_scale_x = 2.0f / (float)walls_needed;

		// Local X range
		float local_start_x = -1.0f + (local_wall_scale_x * 0.5f);
		float local_end_x = 1.0f - (local_wall_scale_x * 0.5f);

		int total_segs_needed = walls_needed * layers_needed;

		// --- POOLING & CLEANUP ---
		if (total_segs_needed != this->renderObjects.size()) {
			for (auto& obj : this->renderObjects) obj->Destroy();
			this->renderObjects.clear();
			type1_renderers.ResetPool();
		}

		type1_renderers.ResetGetIndex();

		// --- PLACEMENT LOOP ---
		for (int i = 0; i < total_segs_needed; i++)
		{
			int x = i % walls_needed;
			int y = i / walls_needed;

			// Calculate Local Position
			float t_x = (walls_needed > 1) ? (float)x / (float)(walls_needed - 1) : 0.5f;
			float t_y = (layers_needed > 1) ? (float)y / (float)(layers_needed - 1) : 0.5f;

			// If there's only 1 segment, t will be 0/0 (NaN), handled by the ternary above
			float local_pos_x = (walls_needed > 1) ? glm::mix(local_start_x, local_end_x, t_x) : 0.0f;
			float local_pos_y = (layers_needed > 1) ? glm::mix(local_start_y, local_end_y, t_y) : 0.5f;

			RenderObject* renderertomove = type1_renderers.Get();

			// Use Local() instead of World() for positions
			// We assume these renderers are parented to 'this'
			renderertomove->Local().position.Set(Vector3(local_pos_x, local_pos_y, 0.0f));

			// Scale is also local relative to the [-1, 1] x [0, 1] parent space
			// Note: z scale should probably be thickness/handle_scale.z if you want absolute thickness
			renderertomove->Local().scale.Set(Vector3(local_wall_scale_x, local_wall_scale_y, 1.0f));

			// Cache indices for the facade/special wall logic
			this->renderObjects[i] = renderertomove;
			object_floors[renderertomove] = y;
			object_cols[renderertomove] = x;
		}

		this->cur_width = walls_needed;
		this->cur_height = layers_needed;
	}
}