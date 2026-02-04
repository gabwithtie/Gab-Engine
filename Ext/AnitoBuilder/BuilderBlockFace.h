#pragma once

#include "Engine/gbe_engine.h"
#include "Math/gbe_math.h"
#include "Editor/gbe_editor.h"

#include <array>

namespace gbe::ext::AnitoBuilder {
	class BuilderBlock;

	class BuilderBlockFace : public Object {
	private:
		bool is_edge = true;

		//user
		std::unordered_map<int, int> floor_mesh_overrides;
		std::unordered_map<int, int> floor_texture_overrides;
		
		bool allow_special_walls = true;
		bool is_backside = false;

		float width_per_wall;
		float height_per_wall;

		BuilderBlock* root_block = nullptr;
		Object* handle_ro = nullptr;

		std::vector<RenderObject*> renderObjects;


		struct RendererSubpool {
			BuilderBlockFace& owner;
			DrawCall* drawcall;

			std::vector<RenderObject*> renderers;
			int get_index = 0;

			inline RendererSubpool(BuilderBlockFace& _owner) : owner(_owner) {

			}

			inline void ResetGetIndex() {
				get_index = 0;
			}

			inline void ResetPool() {
				renderers.clear();
			}

			inline RenderObject* Get() {
				if (renderers.size() <= get_index) {
					auto newrenderer = new RenderObject(drawcall);
					newrenderer->SetParent(owner.handle_ro);
					owner.renderObjects.push_back(newrenderer);

					renderers.push_back(newrenderer);
				}

				auto toreturn = renderers[get_index];

				get_index++;

				return toreturn;
			}
		};

		RendererSubpool type1_renderers;

		std::unordered_map<RenderObject*, int> object_floors;
		std::unordered_map<RenderObject*, int> object_rows;

		int cur_width = 0;
		int cur_height = 0;
	public:
		inline int GetMeshOverride(int floor) {
			auto it = floor_mesh_overrides.find(floor);

			if(it != floor_mesh_overrides.end())
				return floor_mesh_overrides[floor];

			return 0;
		}

		inline void SetMeshOverride(int floor, int mo) {
			floor_mesh_overrides.insert_or_assign(floor, mo);
		}

		inline int GetTexOverride(int floor) {
			auto it = floor_texture_overrides.find(floor);

			if (it != floor_texture_overrides.end())
				return floor_texture_overrides[floor];

			return 0;
		}

		inline void SetTexOverride(int floor, int mo) {
			floor_texture_overrides.insert_or_assign(floor, mo);
		}

		inline Object* Get_handle_parent() {
			return handle_ro;
		}

		inline bool Get_allow_special_walls() {
			return allow_special_walls;
		}

		inline bool Get_is_backside() {
			return is_backside;
		}

		inline void Set_allow_special_walls(bool value) {
			allow_special_walls = value;
		}

		inline void Set_is_backside(bool value) {
			is_backside = value;
		}

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
			OnLocalTransformationChange(TransformChangeType::ALL);

			for (const auto& renderer : renderObjects)
			{
				renderer->Set_enabled(value);
			}
		}
		BuilderBlockFace(BuilderBlock* root_block);
		void OnLocalTransformationChange(TransformChangeType type) override;
	};
}
