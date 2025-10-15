#pragma once

#include "Engine/gbe_engine.h"
#include "Math/gbe_math.h"

namespace gbe::ext::AnitoBuilder {
	class BuilderBlockSet;

	typedef std::pair<int, int> SetSeg;
	struct BlockSet {
		SetSeg seg;
		int handleindex;
	};

	struct BlockSeg {
		SetSeg setseg;
		BuilderBlockSet* block;
	};

	typedef std::pair<Object*, Object*> SetRoof;
	struct BlockSet {
		std::vector<BlockSeg> segs;
		SetRoof roof;
	};

	class BuilderBlock : public Object, public Update {
	public:
		//GEN PARAMS
		float min_dist = 2;
		float max_dist = 20;
		float wall_max_width = 2.5f;
		float wall_max_height = 4;

		float height;

		//IMPORT PARAMS
		float wall_import_height_from_zero = 1.85f;
		float wall_import_width = 1.2f;

		//Drawcalls
		gfx::DrawCall* Wall1_DC;
		gfx::DrawCall* Wall2_DC;
		gfx::DrawCall* roof_editor_DC;
		gfx::DrawCall* roof_DC;

		//Objects
		Object* renderer_parent;
		std::vector<RenderObject*> renderers;

		static void SetModelShown(bool value);
		inline static void ToggleModel() {
			SetModelShown(!model_shown);
		}
	private:
		//WORKING DATA
		static bool model_shown;

		std::vector<BlockSet> sets;
		std::vector<BuilderBlockSet*> handle_pool;
		std::vector<Vector3> position_pool;

		void UpdateModelShown();
		void UpdateHandleSegment(int s, int i, Vector3& l, Vector3& r);

		inline BlockSeg& GetHandle(int s, int i) {
			i %= this->sets[s].segs.size();

			return this->sets[s].segs[i];
		}
		bool CheckSetSegment(Vector3 p, Vector3 l, Vector3 r);
		void SetSegment(int set, int index, int point_index, Vector3 newpoint);
		void ResetHandle(int set, int index);
		inline void SetPosition(int index, Vector3& newpos) {
			const Vector3& oldpos = position_pool[index];

			Vector3 delta = oldpos - newpos;
			if (delta.SqrMagnitude() < 0.001)
				return;

			Vector3 offset = delta.Normalize() * 0.05f;

			position_pool[index] = newpos + offset;

			return;
		}
	public:
		BuilderBlock(gbe::Vector3 corners[4], float height);
		void InvokeUpdate(float deltatime) override;
		inline void AddBlock(BuilderBlockSet* root_handle) {
			for (size_t i = 0; i < handle_pool.size(); i++)
			{
				if(handle_pool[i] == root_handle) {
					AddBlock(i);
					return;
				}
			}
		}
		void AddBlock(int root_handle);
		void AddBlock(int corners[4]);

		SerializedObject Serialize() override;
	};
}
