#pragma once

#include "Engine/gbe_engine.h"
#include "Math/gbe_math.h"

namespace gbe::ext::AnitoBuilder {
	class BuilderBlockSet;

	typedef std::pair<std::pair<int, int>, BuilderBlockSet*> BlockSet;
	typedef std::pair<int, int> SetSeg;

	class BuilderBlock : public Object, public Update {
	private:
		std::vector<std::vector<BlockSet>> sets;
		std::vector<Vector3> position_pool;

		float height;
		
		void UpdateHandleSegment(int s, int i);

		inline BlockSet& GetHandle(int s, int i) {
			i %= this->sets[s].size();

			return this->sets[s][i];
		}
		void SetSegment(int set, int index, int point_index, Vector3 newpoint);
	public:
		BuilderBlock(gbe::Vector3 corners[4], float height);
		void InvokeUpdate(float deltatime) override;
		void AddBlock(BuilderBlockSet* root_handle);
		void AddBlock(int corners[4], BuilderBlockSet* root_handle = nullptr);
	};
}
