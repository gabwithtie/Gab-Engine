#pragma once

#include <Extension/Extension.h>

namespace gbe::ext::AnitoBuilder {
	class AnitoBuilderExtension : public Extension {
	public:
		static bool unit_select;

		virtual void OnEngineInitialize() override;
		virtual void OnEngineRunLoopStart() override;
		virtual void OnEngineRunLoopEnd() override;
		virtual void OnEngineShutdown() override;
	};
}