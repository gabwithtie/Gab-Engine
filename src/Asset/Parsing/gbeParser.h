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
				inline static bool ValidateDirectory(std::filesystem::path directory) {
					// Get the parent path (which is the folder/directory)
					std::filesystem::path folderPath = directory.parent_path();

					if (std::filesystem::create_directory(folderPath)) {
						return false;
					}
					else {
						return true;
					}
				}

				template<class TImportData>
				static void PopulateClassStr(TImportData& target, std::string data) {
					TImportData datareceiver;
					auto ec = glz::read < glz::opts{ .error_on_unknown_keys = false } > (datareceiver, data);
					target = datareceiver;
					std::cout << ec << std::endl;
				}

				template<class TExportData>
				static std::string ExportClassStr(TExportData& target) {
					auto ec = glz::write_json(target);
					return ec;
				}

				template<class TImportData>
				static bool PopulateClass(TImportData& target, std::filesystem::path asset_path) {
					std::string buffer;
					TImportData datareceiver;
					auto ec = glz::read_file_json<glz::opts{.error_on_unknown_keys = false}>(datareceiver, asset_path.string(), buffer);
					target = datareceiver;

					if (ec) {
						std::cout << ec << std::endl;
						return false;
					}

					return true;
				}

				template<class TExportData>
				static void ExportClass(TExportData& target, std::filesystem::path asset_path) {
					ValidateDirectory(asset_path);
					std::ofstream file(asset_path);
					auto ec = glz::write_json(target);
					file.close();
				}
			};
		}
	}
}