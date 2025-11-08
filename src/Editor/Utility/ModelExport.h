#pragma once

#include "Graphics/gbe_graphics.h"
#include "Engine/gbe_engine.h"

#include "Asset/AssetTypes/Mesh.h"

// Assimp includes for the export class
#include <assimp/scene.h>
#include <assimp/Exporter.hpp>

namespace gbe {
	namespace editor {
		class ModelExport {
			std::vector<gbe::Object*> selected;

			struct export_subobject {
				const asset::data::MeshLoadData* source_mesh;
				Matrix4 transform;
			};
		public:
			ModelExport(std::vector<gbe::Object*> selected);
			void Export(std::filesystem::path path);
		};
	}
}