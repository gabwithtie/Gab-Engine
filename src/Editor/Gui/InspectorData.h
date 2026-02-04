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
				CHOICE
			};
			
			std::function<void()> onchange;
			std::string name;
			FieldType fieldtype;
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

		struct InspectorBool : public InspectorField {
			bool* x = nullptr;

			InspectorBool() {
				this->fieldtype = FieldType::BOOLEAN;
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

		struct InspectorChoice : public InspectorField {
			int* index;
			std::vector<std::string> *labels;

			InspectorChoice() {
				this->fieldtype = FieldType::CHOICE;
			}
		};

		struct InspectorData {
			std::vector<InspectorField*> fields;
		};
	}
}