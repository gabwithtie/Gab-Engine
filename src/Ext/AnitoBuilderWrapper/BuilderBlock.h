#pragma once

#include "Engine/gbe_engine.h"
#include "Math/gbe_math.h"

namespace gbe::ext::AnitoBuilder {
	class BuilderBlock : public Object, public Update {
	private:
		std::vector < std::pair<Vector3, Vector3> > segments;
		float height;

		std::vector<Object*> handles;
		
		inline std::pair<Vector3, Vector3>& GetSegment(int index) {
			index %= segments.size();

			return segments[index];
		}

		void RecalculateTransformations(int index);
	public:
		BuilderBlock(gbe::Vector3 corners[4], float height);
		void InvokeUpdate(float deltatime) override;
	};
}
