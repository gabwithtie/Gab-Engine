#pragma once

#include "Engine/gbe_engine.h"
#include "Math/gbe_math.h"

namespace gbe::ext::AnitoBuilder {
	class BuilderBlockSet;

	typedef std::pair<std::pair<int, int>, BuilderBlockSet*> BlockSet;
	typedef std::pair<int, int> SetSeg;

	class BuilderBlock : public Object, public Update {
	public:
		//PARAMS
		float min_dist = 2;
		float max_dist = 20;
		float wall_max_width = 3;
		float wall_max_height = 3;
		float height;

	private:
		//WORKING DATA
		std::vector<std::vector<BlockSet>> sets;
		std::vector<Vector3> position_pool;

		
		void UpdateHandleSegment(int s, int i, Vector3& l, Vector3& r);

		inline BlockSet& GetHandle(int s, int i) {
			i %= this->sets[s].size();

			return this->sets[s][i];
		}
		bool CheckSetSegment(int set, int index, int point_index, Vector3 newpoint_a, Vector3 newpoint_b);
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
		void AddBlock(BuilderBlockSet* root_handle);
		void AddBlock(int corners[4], BuilderBlockSet* root_handle = nullptr);
	};
}
