#pragma once

#include <typeinfo>
#include <unordered_map>
#include <string>
#include <functional>

#include "Engine/gbe_engine.h"

#include <nfd.h>
#include <stdio.h>
#include <stdlib.h>

namespace gbe::editor {
	class FileDialogue {
	public:
		enum OpType {
			OPEN,
			SAVE,
			FOLDER
		};

		inline static std::string GetFilePath(OpType optype, std::string extension = "") {
			nfdu8char_t* outPath;
			std::string outPathStr = "";
			
			nfdresult_t result = {};
			
			switch (optype)
			{
			case gbe::editor::FileDialogue::OPEN: {
				nfdopendialogu8args_t args = { 0 };
				args.filterCount = 0;
				result = NFD_OpenDialogU8_With(&outPath, &args);
				break;
			}
			case gbe::editor::FileDialogue::SAVE: {
				nfdsavedialogu8args_t args = { 0 };
				args.filterCount = 0;
				result = NFD_SaveDialogU8_With(&outPath, &args);
				break;
			}
			case gbe::editor::FileDialogue::FOLDER: {

				break;
			}
			}

			if (result == NFD_OKAY)
			{
				outPathStr = std::string(outPath);
				NFD_FreePathU8(outPath);

				if(extension.size() > 0)
				if (outPathStr.ends_with(extension) == false)
					return "";
			}
			else if (result == NFD_CANCEL)
			{
				return "";
			}
			else
			{
				printf("Error: %s\n", NFD_GetError());
			}

			return outPathStr;
		}
	};
}