#pragma once

#include "Engine/gbe_engine.h"
#include "Math/gbe_math.h"
#include "Editor/gbe_editor.h"

namespace gbe::ext::AnitoBuilder {
	class BuilderBlock;

	class BuilderBlockSet : public Object {
	private:
		BuilderBlock* root_block = nullptr;
	public:
		inline BuilderBlockSet(BuilderBlock* root_block) {
			this->root_block = root_block;

			this->SetName("Anito Builder Block Set");

			auto add_block_button = new gbe::editor::InspectorButton();
			add_block_button->name = "Append Block";
			add_block_button->onpress = [=]() {
				root_block->AddBlock(this);
				};

			this->inspectorData->fields.push_back(add_block_button);
		}
	};
}
