#pragma once

#include "Editor/Gui/InspectorData.h"
#include "Editor/Gui/Windows/GuiWindow.h"

#include "Engine/gbe_engine.h"

#include <string>

namespace gbe::ext::AnitoBuilder{
	class AnitoBuilderExtension;
}

namespace gbe {
	namespace editor {
		class AnitoBuilderWindow : public GuiWindow {
			gbe::ext::AnitoBuilder::AnitoBuilderExtension* ext;
			void DrawSelf() override;
			void DrawChildList(Object* parent, std::string label, unsigned int id = 0);
		public:
			AnitoBuilderWindow(gbe::ext::AnitoBuilder::AnitoBuilderExtension* _ext);
			std::string GetWindowId() override;
		};
	}
}