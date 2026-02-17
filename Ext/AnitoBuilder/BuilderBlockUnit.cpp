#include "BuilderBlockUnit.h"

#include "BuilderBlockFace.h"
#include "BuilderBlock.h"

gbe::ext::AnitoBuilder::BuilderBlockUnit::BuilderBlockUnit(BuilderBlockFace* parent, int _pos)
{
	{
		static std::vector<std::string> labels;

		auto root_block = parent->Get_root_block();

		for (size_t i = 0; i < root_block->GetOverrideTypeCount(); i++)
		{
			labels.push_back(std::to_string(i));
		}

		auto field = new gbe::editor::InspectorChoice();
		field->labels = &labels;
		field->getter = [=]() {
			auto segdata = root_block->GetSeg(parent);

			if (segdata != nullptr)
			{
				auto it = segdata->dc_overrides.find(_pos);

				if (it != segdata->dc_overrides.end())
					return it->second;

				return -1;
			}

			return 0;
			};
		field->setter = [=](int val) {
			auto segdata = root_block->GetSeg(parent);
			
			segdata->dc_overrides.insert_or_assign(_pos, val);

			root_block->Refresh();
			};
		field->name = "mesh_override";

		this->inspectorData->fields.push_back(field);
	}
}
