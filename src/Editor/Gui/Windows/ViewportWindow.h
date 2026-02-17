#pragma once

#include "GuiWindow.h"
#include "../GizmoLayer.h"
#include <string>
#include <vector> // Required for caching
#include "Math/gbe_math.h"
#include "Graphics/gbe_graphics.h"

namespace gbe {
    class Object;

    namespace editor {
        // Data structure to store icon calls
        struct IconCommand {
            gfx::TextureData* tex;
            Vector3 position;
            Matrix4 view;
            Matrix4 proj;
        };

        class ViewportWindow : public GuiWindow {
            void DrawSelf() override;

            gfx::TextureData* selected_data = nullptr;
            Vector2Int old_resolution;
            ImVec2 viewportScreenPos; // Cached for coordinate math
            ImVec2 viewportSize;      // Cached for coordinate math

            static std::vector<IconCommand> iconQueue; // The command buffer
            editor::GizmoLayer gizmoLayer;

        protected:
            inline void push_styles() override {
                ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 0.0f , 0.0f });
            }
            inline void pop_styles() override { ImGui::PopStyleVar(); }

        public:
            inline std::string GetWindowId() override { return "ViewportWindow"; }
            ViewportWindow(std::vector<gbe::Object*>& _selected);

            // This now just adds to the queue
            static void RenderIcon(gfx::TextureData* tex, const Vector3& position, const Matrix4& camera_view, const Matrix4& camera_proj);
        };
    }
}