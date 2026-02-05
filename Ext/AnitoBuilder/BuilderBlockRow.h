#pragma once

#include "Engine/gbe_engine.h"
#include "Math/gbe_math.h"
#include "Editor/gbe_editor.h"

#include <array>

namespace gbe::ext::AnitoBuilder {
	class BuilderBlock;
	class BuilderBlockFace;

	class BuilderBlockRow : public Object {
		std::vector<std::vector<gbe::gfx::DrawCall*>> choices;
		std::vector<std::string> mesh_labels;
		std::vector<std::string> tex_labels;
		int rownum = 0;

		BuilderBlock* root;
		BuilderBlockFace* from;
	public:
		DrawCall* GetOverrideDrawCall();

		BuilderBlockRow(BuilderBlock* _root, BuilderBlockFace* _from, int _rownum);
		void GeneralInit() override;
	};
}