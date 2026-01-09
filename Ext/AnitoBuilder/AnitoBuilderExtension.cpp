#include "AnitoBuilderExtension.h"

#include "gui/AnitoBuilderWindow.h"
#include "BuilderBlock.h"

#include <typeinfo>

void gbe::ext::AnitoBuilder::AnitoBuilderExtension::OnEngineInitialize()
{
	this->extension_windows.push_back(new gbe::editor::AnitoBuilderWindow());

	gbe::TypeSerializer::RegisterTypeCreator(typeid(gbe::ext::AnitoBuilder::BuilderBlock).name(), [](SerializedObject* data) {return new ext::AnitoBuilder::BuilderBlock(data); });
}

void gbe::ext::AnitoBuilder::AnitoBuilderExtension::OnEngineRunLoopStart()
{
}

void gbe::ext::AnitoBuilder::AnitoBuilderExtension::OnEngineRunLoopEnd()
{
}

void gbe::ext::AnitoBuilder::AnitoBuilderExtension::OnEngineShutdown()
{
}
