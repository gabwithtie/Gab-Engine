#include "ContextMenus.h"

void gbe::editor::ContextMenus::GenericObject(gbe::Object* obj)
{
	if (ImGui::BeginPopupContextItem()) {
		
		if (ImGui::MenuItem("Delete", "Del")) {
			auto parent_id = obj->GetParent()->Get_id();
			auto destroy_id = obj->Get_id();
			auto respawn_info = obj->Serialize();

			Editor::DeselectAll();

			Editor::CommitAction(
				[=]() {
					Engine::GetCurrentRoot()->GetObjectWithId(destroy_id)->Destroy();
				},
				[=]() {
					auto info = respawn_info;

					auto undoed = gbe::TypeSerializer::Instantiate(respawn_info.type, &info);
					undoed->Set_id(destroy_id);
					undoed->SetParent(Engine::GetCurrentRoot()->GetObjectWithId(parent_id));
				}
			);
		}

		ImGui::EndPopup();
	}
}
