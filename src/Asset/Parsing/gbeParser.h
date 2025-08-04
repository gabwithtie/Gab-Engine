#pragma once

#include <glaze/glaze.hpp>
#include <string>
#include <iostream>
#include <sstream>
#include <fstream>
#include <filesystem>

namespace gbe {
	namespace asset {
		namespace serialization {
			class gbeParser {
			public:
				inline static bool ValidateDirectory(std::string directory) {
					// Create a std::filesystem::path object from the string
					std::filesystem::path p(directory);

					// Get the parent path (which is the folder/directory)
					std::filesystem::path folderPath = p.parent_path();

					if (std::filesystem::create_directory(folderPath)) {
						return false;
					}
					else {
						return true;
					}
				}

				template<class TImportData>
				static void PopulateClass(TImportData& target, std::string asset_path) {
					std::string buffer;
					TImportData datareceiver;
					auto ec = glz::read_file_json<glz::opts{.error_on_unknown_keys = false}>(datareceiver, asset_path, buffer);
					target = datareceiver;
					std::cout << ec << std::endl;
				}

				template<class TExportData>
				static void ExportClass(TExportData& target, std::string asset_path) {
					ValidateDirectory(asset_path);
					std::ofstream file(asset_path);
					auto ec = glz::write_json(target);
					file << ec;
					file.close();
				}
			};
		}
	}
}