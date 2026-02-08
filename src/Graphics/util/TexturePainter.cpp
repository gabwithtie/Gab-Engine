#include "TexturePainter.h"
#include "Graphics/RenderPipeline.h"

namespace gbe {
	namespace gfx {
		TexturePainter* TexturePainter::instance = nullptr;
		bool TexturePainter::enabled = false;

		void TexturePainter::SetEnabled(bool en)
		{
			instance->enabled = en;

			if (en) {
				RenderPipeline::GetRenderer()->SetCpuPassMode(Renderer::CPU_PASS_MODE::PASS_UV);
			}
			else {
				RenderPipeline::GetRenderer()->SetCpuPassMode(Renderer::CPU_PASS_MODE::PASS_ID);
			}
		}

		void TexturePainter::Draw(Vector2Int pos)
		{
			if (!instance)
				return;

			if (instance->draw_callback)
				instance->draw_callback(pos);
		}

		void TexturePainter::Commit()
		{
			if (!instance)
				return;
			if (!instance->commit_callback)
				return;

			for (const auto& cmd : instance->draw_queue)
			{
				instance->commit_callback(cmd);
			}

			instance->draw_queue.clear();
		}
	}
}