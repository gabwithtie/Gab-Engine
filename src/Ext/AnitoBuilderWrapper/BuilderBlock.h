#pragma once

#include "Engine/gbe_engine.h"
#include "Math/gbe_math.h"

#include <array>

namespace gbe::ext::AnitoBuilder {
	class BuilderBlockSet;

	typedef std::pair<int, int> SetSeg;

	struct BlockSeg {
		SetSeg seg;
		int handleindex;
	};

	struct BlockSet {
		std::vector<BlockSeg> segs;
	};

	struct SetRoof {
		std::array<gbe::Object*, 2> objs;
		int parent_index;
	};

	struct BuilderBlockData {
		std::vector<std::array<float, 3>> positions;
		std::vector<BlockSet> sets;

		Vector3 GetPosition(int index) {
			if (index < 0 || index >= positions.size())
				return Vector3(0, 0, 0);
			return Vector3(positions[index][0], positions[index][1], positions[index][2]);
		}
		void SetPosition(int index, Vector3 data) {
			if (index < 0 || index >= positions.size())
				return;
			positions[index][0] = data.x;
			positions[index][1] = data.y;
			positions[index][2] = data.z;
		}
		void AddPosition(Vector3 data) {
			positions.push_back({ data.x, data.y, data.z });
		}
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
	protected:
		void InitializeInspectorData() override;
	private:
		//WORKING DATA
		static bool model_shown;

		BuilderBlockData data;

		std::vector<BuilderBlockSet*> handle_pool;
		std::vector<SetRoof> roof_pool;

		void UpdateModelShown();
		void UpdateHandleSegment(int s, int i, Vector3& l, Vector3& r);

		inline BlockSeg& GetHandle(int s, int i) {
			i %= this->data.sets[s].segs.size();

			return this->data.sets[s].segs[i];
		}
		bool CheckSetSegment(Vector3 p, Vector3 l, Vector3 r);
		void ResetHandle(int set, int index);
		inline void SetPosition(int index, Vector3& newpos) {
			const Vector3& oldpos = data.GetPosition(index);

			Vector3 delta = oldpos - newpos;
			if (delta.SqrMagnitude() < 0.001)
				return;

			Vector3 offset = delta.Normalize() * 0.05f;

			data.SetPosition(index, newpos + offset);

			return;
		}
	public:
		BuilderBlock(gbe::Vector3 corners[4], float height);
		BuilderBlock(SerializedObject* data);
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
