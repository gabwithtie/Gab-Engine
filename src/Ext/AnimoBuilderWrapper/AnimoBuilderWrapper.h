#pragma once

#include "Engine/Objects/Object.h"
#include "Graphics/Data/DrawCall.h"
#include "ext/AnimoBuilder/AnimoBuilder.h"

#include <vector>

namespace gbe {
	namespace ext {
		namespace AnimoBuilder {
			class AnimoBuilderObject : public gbe::Object {
			private:
				gbe::Object* generation_root = nullptr;
				gfx::DrawCall* cube_drawcall = nullptr;
				std::unordered_map<std::string, gfx::DrawCall*> drawcall_dictionary;
				ext::AnimoBuilder::GenerationParams parentparams;

				void Regenerate();
				void CreateMesh(gfx::DrawCall* drawcall, Vector3 pos, Vector3 scale, Quaternion rotation = Quaternion::Euler(Vector3(0, 0, 0)));
			public:
				AnimoBuilderObject();
				void AddPillar(Vector3 position);
				inline void SetBaseParams(ext::AnimoBuilder::GenerationParams _parentparams) {
					this->parentparams = _parentparams;
				}
			};
		}
	}
}