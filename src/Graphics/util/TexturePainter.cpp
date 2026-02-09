#include "TexturePainter.h"
#include "Graphics/RenderPipeline.h"

namespace gbe {
	namespace gfx {
		TexturePainter* TexturePainter::instance = nullptr;
		bool TexturePainter::enabled = false;

		void TexturePainter::SetEnabled(bool en)
		{
			instance->enabled = en;
		}

		void TexturePainter::Draw(Vector2Int pos)
		{
			if (!instance)
				return;
			if (instance->target_texture == nullptr)
				return;

			if (instance->impl_draw)
				instance->impl_draw(pos);
		}
	}
}