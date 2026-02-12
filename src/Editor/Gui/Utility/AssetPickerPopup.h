#pragma once

#include <imgui.h>
#include <vector>
#include "Graphics/gbe_graphics.h"
#include <string>
#include <algorithm>
#include <functional>
#include "Asset/gbe_asset.h"
#include "DragDrop.h"

namespace gbe::editor {

    inline bool AssetPickerPopup(
        const char* popupName,
        asset::AssetType assettype,
        std::function<asset::internal::BaseAsset_base* ()> getter,
        std::function<void(std::string)> setter,
        const std::vector<std::string>& assetList) {
        bool selected = false;

        // Use a fixed size for the search buffer
        static char searchBuffer[128] = "";
        auto cur_asset = getter();

        if (ImGui::BeginPopup(popupName)) {
            // --- 1. Search Bar ---
            ImGui::Text("Search:");
            ImGui::SameLine();
            if (ImGui::InputText("##filter", searchBuffer, IM_ARRAYSIZE(searchBuffer))) {
                // Search buffer updated
            }

            ImGui::Separator();

            // --- 2. Scrollable Selection Area ---
            // Height is set to ~200 pixels; width 0 means "use remaining"
            if (ImGui::BeginChild("AssetList", ImVec2(300, 200), true)) {
                std::string filter = searchBuffer;
                std::transform(filter.begin(), filter.end(), filter.begin(), ::tolower);

                for (const auto& asset : assetList) {
                    // Simple case-insensitive search logic
                    std::string assetLower = asset;
                    std::transform(assetLower.begin(), assetLower.end(), assetLower.begin(), ::tolower);

                    if (filter.empty() || assetLower.find(filter) != std::string::npos) {
                        bool selected_cur = false;
                        if (cur_asset != nullptr)
                            selected_cur = cur_asset->Get_assetId() == asset;

                        if (ImGui::Selectable(asset.c_str(), selected_cur)) {
                            setter(asset);
                            selected = true;
                            ImGui::CloseCurrentPopup(); // Optional: close on click
                        }
                    }
                }
                ImGui::EndChild();
            }

            // --- 3. Footer ---
            if (ImGui::Button("Close", ImVec2(120, 0))) {
                ImGui::CloseCurrentPopup();
            }

            ImGui::EndPopup();
        }

        return selected;
    }

    inline bool TextureAssetPicker(
        const char* label,
        const ImVec2& size,
        std::function<asset::Texture* ()> getter,
        std::function<void(std::string)> setter,
        const std::vector<std::string>& assetList)
    {
        bool selected_new = false;

        DraggableTarget(asset::AssetType::TEXTURE,
            [&](const DragData& data) {setter(data.id); },
            [&]() {
                std::string popupId = std::string("Popup_") + label;
                ImGui::BeginGroup();

                ImGui::Text("%s", label);

                auto currentAsset = getter();

                // Default values for the "Empty" state
                bgfx::TextureHandle handle = BGFX_INVALID_HANDLE;
                std::string displayId = "empty";

                // 1. Null Check & Data Extraction
                if (currentAsset != nullptr) {
                    displayId = currentAsset->Get_assetId();
                    auto assetData = TextureLoader::GetAssetRuntimeData(displayId);
                    if (assetData) {
                        handle = assetData->textureHandle;
                    }
                }

                // 2. Prepare Texture ID for ImGui
                ImTextureID texId = (ImTextureID)(uintptr_t)handle.idx;

                

                // 5. Integrate Popup logic
                // We capture 'currentAsset' - if it's null, the lambda returns nullptr safely
                const auto base_getter = [currentAsset]() -> asset::internal::BaseAsset_base* {
                    return static_cast<asset::internal::BaseAsset_base*>(currentAsset);
                    };

                // 3. Draw the Image Button
                // We use the label as the ID for the button to ensure unique interaction
                const auto& openfunc = [&]() {
                    return ImGui::OpenPopup(popupId.c_str());
                    };


                if (currentAsset != nullptr) {
                    if (ImGui::ImageButton(label, texId, size, ImVec2(0, 0), ImVec2(1, 1)))
                        openfunc();
                }
                else
                    if (ImGui::Button(label, size))
                        openfunc();
                    

                // 4. Display text (shows "ID: empty" if null)
                ImGui::TextWrapped("ID: %s", displayId.c_str());

                AssetPickerPopup(popupId.c_str(), asset::TEXTURE, base_getter, setter, assetList);

                ImGui::EndGroup();
            });

        return selected_new;
    }
}