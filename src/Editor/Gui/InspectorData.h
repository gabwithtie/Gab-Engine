#pragma once

#include <string>
#include <vector>
#include <functional>

#include "Asset/gbe_asset.h"
#include "Math/gbe_math.h"

namespace gbe {
	namespace editor {
		enum FieldType {
			STRING,
			INT,
			FLOAT,
			VECTOR3,
			QUATERNION,
			COLOR,
			BOOLEAN,
			FUNCTION,
			CHOICE,
			TEXTURE,
			DICTIONARY,
			ASSET
		};

		struct InspectorField_base {
			std::string name;
			FieldType fieldtype;
		};

		template<typename T>
		struct InspectorField : public InspectorField_base {
			std::function<T()> getter;
			std::function<void(T)> setter;
		};

		struct InspectorVec3 : public InspectorField<Vector3> {

			InspectorVec3() {
				this->fieldtype = FieldType::VECTOR3;
			}
		};

		struct InspectorColor : public InspectorField<Vector3> {
			InspectorColor() {
				this->fieldtype = FieldType::COLOR;
			}
		};

		struct InspectorFloat : public InspectorField<float> {
			InspectorFloat() {
				this->fieldtype = FieldType::FLOAT;
			}
		};

		struct InspectorAssetDictionary : public InspectorField<int> {
			std::function<asset::internal::BaseAsset_base*(std::string)> a_getter;
			std::function<void(std::string, asset::internal::BaseAsset_base*)> a_setter;
			std::vector<std::string> *fieldList;
			asset::AssetType assettype;
			InspectorAssetDictionary() {
				this->fieldtype = FieldType::DICTIONARY;
			}
		};

		struct InspectorAsset : public InspectorField<std::string> {
			asset::AssetType assettype;
			InspectorAsset() {
				this->fieldtype = FieldType::ASSET;
			}
		};

		struct InspectorBool : public InspectorField<bool> {
			InspectorBool() {
				this->fieldtype = FieldType::BOOLEAN;
			}
		};

		struct InspectorString : public InspectorField<std::string> {
			InspectorString() {
				this->fieldtype = FieldType::STRING;
			}
		};

		struct InspectorTexture : public InspectorField<std::string> {
			InspectorTexture() {
				this->fieldtype = FieldType::TEXTURE;
			}
		};

		struct InspectorButton :public InspectorField<bool> {
			std::function<void()> onpress;

			InspectorButton() {
				this->fieldtype = FieldType::FUNCTION;
			}
		};

		struct InspectorChoice : public InspectorField<int> {
			std::vector<std::string> *labels;

			InspectorChoice() {
				this->fieldtype = FieldType::CHOICE;
			}
		};

		struct InspectorData {
			std::vector<InspectorField_base*> fields;
		};
	}
}