#pragma once

#include "Engine/gbe_engine.h"
#include "Math/gbe_math.h"

namespace gbe::ext::AnitoBuilder {
	class BuilderBlockSet;

	class BuilderBlock : public Object, public Update {
	private:
		std::vector<std::vector<std::pair<Vector3, Vector3>>> sets;
		float height;

		std::vector<std::vector<BuilderBlockSet*>> handles;
		
		inline std::pair<Vector3, Vector3>& GetSegment(int set, int index) {
			index %= sets[set].size();

			return sets[set][index];
		}

		void RecalculateTransformations(BuilderBlockSet*);
	public:
		BuilderBlock(gbe::Vector3 corners[4], float height);
		void InvokeUpdate(float deltatime) override;
		void AddBlock(BuilderBlockSet* root_handle);
		void AddBlock(Vector3 corners[4], BuilderBlockSet* root_handle = nullptr);
	};
}
