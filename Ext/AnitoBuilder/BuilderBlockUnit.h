#pragma once

#include "Engine/gbe_engine.h"
#include "Math/gbe_math.h"
#include "Editor/gbe_editor.h"

#include <array>

namespace gbe::ext::AnitoBuilder {
	class BuilderBlockFace;
	class BuilderBlock;
	class BlockFace;

	class BuilderBlockUnit : public Object {
	public:
		BuilderBlockUnit(BuilderBlockFace* parent, int _pos);
	};
}