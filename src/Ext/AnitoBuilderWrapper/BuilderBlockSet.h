#pragma once

#include "Engine/gbe_engine.h"
#include "Math/gbe_math.h"
#include "Editor/gbe_editor.h"

namespace gbe::ext::AnitoBuilder {
	class BuilderBlock;

	class BuilderBlockSet : public Object {
	private:
		BuilderBlock* root_block = nullptr;
		RigidObject* handle_ro = nullptr;

		std::vector<RenderObject*> renderObjects;
	public:
		BuilderBlockSet(BuilderBlock* root_block);
		void OnLocalTransformationChange(TransformChangeType type) override;
	};
}
