#include "ScreenUtil.h"

static bool initialized_screen = false;

bgfx::VertexLayout gbe::gfx::bgfx_gab::ScreenVertex::ms_layout;

void gbe::gfx::bgfx_gab::RenderFullscreenPass(bgfx::ViewId _view, bgfx::ProgramHandle _program, uint64_t flag)
{
    if (!initialized_screen) {
        // Initialize Screen Vertex Layout
        ScreenVertex::ms_layout
            .begin()
            .add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
            .add(bgfx::Attrib::TexCoord0, 2, bgfx::AttribType::Float)
            .end();

        initialized_screen = true;
    }

    if (3 == bgfx::getAvailTransientVertexBuffer(3, ScreenVertex::ms_layout)) {
        bgfx::TransientVertexBuffer vb;
        bgfx::allocTransientVertexBuffer(&vb, 3, ScreenVertex::ms_layout);

        ScreenVertex* vertex = (ScreenVertex*)vb.data;
        // Large triangle covering the whole screen [-1, 3] and [-1, 3]
        vertex[0] = { -1.0f, -1.0f, 0.0f, 0.0f, 1.0f };
        vertex[1] = { 3.0f, -1.0f, 0.0f, 2.0f, 1.0f };
        vertex[2] = { -1.0f,  3.0f, 0.0f, 0.0f, -1.0f };

        bgfx::setVertexBuffer(0, &vb);
        bgfx::setState(BGFX_STATE_WRITE_RGB | flag | BGFX_STATE_WRITE_A);
        bgfx::submit(_view, _program);
    }
}
