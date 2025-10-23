#pragma once

#include "Engine/gbe_engine.h"
#include "Math/gbe_math.h"
#include "Editor/gbe_editor.h"

namespace gbe::ext::AnitoBuilder {
	class BuilderBlock;

	class BuilderBlockSet : public Object {
	private:
		bool is_edge = true;

		float width_per_wall;
		float height_per_wall;

		BuilderBlock* root_block = nullptr;
		RigidObject* handle_ro = nullptr;

		std::vector<RenderObject*> renderObjects;
		std::unordered_map<RenderObject*, int> object_floors;
		std::unordered_map<RenderObject*, int> object_rows;

		int cur_width = 0;
		int cur_height = 0;
	public:
		inline float Get_width_per_wall() {
			return width_per_wall;
		}

		inline float Get_height_per_wall() {
			return height_per_wall;
		}

		inline int Get_floor(RenderObject* obj)
		{
			return object_floors[obj];
		}

		inline int Get_row(RenderObject* obj)
		{
			return object_rows[obj];
		}

		inline int Get_cur_width()
		{
			return cur_width;
		}

		inline int Get_cur_height()
		{
			return cur_height;
		}

		inline std::vector<RenderObject*>& Get_all_segs() {
			return renderObjects;
		}

		inline void Set_is_edge(bool value) {
			is_edge = value;
		}

		inline bool Get_is_edge() {
			return is_edge;
		}

		void Set_visible(bool value) {
			if (!value) {
				for (const auto& renderer : renderObjects)
				{
					renderer->Set_enabled(false);
				}
			}
			else
			{
				for (const auto& renderer : renderObjects)
				{
					renderer->Destroy();
				}
				renderObjects.clear();

				OnLocalTransformationChange(gbe::ALL);
			}
		}
		BuilderBlockSet(BuilderBlock* root_block);
		void OnLocalTransformationChange(TransformChangeType type) override;
	};
}
