#pragma once

#include <string>
#include <vector>
#include <functional>

#include "Asset/gbe_asset.h"

namespace gbe {
	namespace editor {
		struct InspectorField {
			enum FieldType {
				STRING,
				INT,
				FLOAT,
				VECTOR3,
				QUATERNION,
				COLOR,
				BOOLEAN,
				FUNCTION,
				ASSET
			};
			
			std::string name;
			FieldType fieldtype;
		};

		struct InspectorAsset_base : public InspectorField {
			std::function<std::vector<gbe::asset::internal::BaseAsset_base*>()> _getter;
			gbe::asset::internal::BaseAsset_base** choice;
			std::string choice_label = "NULL";

			std::vector<gbe::asset::internal::BaseAsset_base*> GetChoices() {
				return _getter();
			}

			InspectorAsset_base() {
				this->fieldtype = FieldType::ASSET;
			}
		};

		template<class TAssetLoader, class TAsset>
		struct InspectorAsset : public InspectorAsset_base {
			TAssetLoader* loader;

			InspectorAsset() {
				this->_getter = [this]() {
					return this->loader->GetAssetList();
					};
			}
		};

		struct InspectorVec3 : public InspectorField {
			float* x = nullptr;
			float* y = nullptr;
			float* z = nullptr;

			InspectorVec3() {
				this->fieldtype = FieldType::VECTOR3;
			}
		};

		struct InspectorColor : public InspectorField {
			float* r = nullptr;
			float* g = nullptr;
			float* b = nullptr;

			InspectorColor() {
				this->fieldtype = FieldType::COLOR;
			}
		};

		struct InspectorFloat : public InspectorField {
			float* x = nullptr;

			InspectorFloat() {
				this->fieldtype = FieldType::FLOAT;
			}
		};

		struct InspectorString : public InspectorField {
			std::string* str = nullptr;

			InspectorString() {
				this->fieldtype = FieldType::STRING;
			}
		};

		struct InspectorButton :public InspectorField {
			std::function<void()> onpress;

			InspectorButton() {
				this->fieldtype = FieldType::FUNCTION;
			}
		};

		struct InspectorData {
			std::vector<InspectorField*> fields;
		};
	}
}