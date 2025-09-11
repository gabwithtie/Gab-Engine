#pragma once

#include "Engine/gbe_engine.h"
#include "Math/gbe_math.h"

namespace gbe::ext::AnitoBuilder {
	class BuilderBlock {
	public:
		static Object* CreateBlock(gbe::Vector3 corners[4], float height);
	};
}
