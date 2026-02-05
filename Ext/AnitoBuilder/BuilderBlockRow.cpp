#include "BuilderBlockRow.h"

#include "BuilderBlock.h"
#include "BuilderBlockFace.h"

gbe::gfx::DrawCall* gbe::ext::AnitoBuilder::BuilderBlockRow::GetOverrideDrawCall()
{
	return this->choices[from->GetTexOverride(rownum)][from->GetMeshOverride(rownum)];
}

gbe::ext::AnitoBuilder::BuilderBlockRow::BuilderBlockRow(BuilderBlock* _root, BuilderBlockFace* _from, int _rownum)
{
	this->root = _root;
	this->from = _from;
	this->rownum = _rownum;

	std::vector<asset::Material*> materials;
	materials.push_back(asset::Material::GetAssetById("plaster"));
	materials.push_back(asset::Material::GetAssetById("c1"));
	materials.push_back(asset::Material::GetAssetById("c3"));

	for (const auto& mat : materials)
	{
		this->tex_labels.push_back(mat->Get_assetId());

		std::vector<DrawCall*> callset;

		callset.push_back(RenderPipeline::RegisterDrawCall(asset::Mesh::GetAssetById("wallnorm2"), mat));
		callset.push_back(RenderPipeline::RegisterDrawCall(asset::Mesh::GetAssetById("fillerwall_1"), mat));
		callset.push_back(RenderPipeline::RegisterDrawCall(asset::Mesh::GetAssetById("fillerwall_2"), mat));
		callset.push_back(RenderPipeline::RegisterDrawCall(asset::Mesh::GetAssetById("fillerwall_3"), mat));
		callset.push_back(RenderPipeline::RegisterDrawCall(asset::Mesh::GetAssetById("fillerwall_4"), mat));
		callset.push_back(RenderPipeline::RegisterDrawCall(asset::Mesh::GetAssetById("fillerwall_5"), mat));

		this->choices.push_back(callset);
	}

	for (const auto& altmesh : this->choices[0])
	{
		this->mesh_labels.push_back(altmesh->get_mesh()->Get_assetId());
	}

	GeneralInit();
}

void gbe::ext::AnitoBuilder::BuilderBlockRow::GeneralInit()
{
	Object::GeneralInit();

	{
		auto field = new gbe::editor::InspectorChoice();
		field->name = "Override Wall";
		field->labels = &this->mesh_labels;
		field->getter = [=]() {return from->GetMeshOverride(rownum); };
		field->setter = [=](int val) {
			from->SetMeshOverride(this->rownum, val);
			root->Refresh();
			};

		this->inspectorData->fields.push_back(field);
	}

	{
		auto field = new gbe::editor::InspectorChoice();
		field->name = "Override Texture";
		field->labels = &this->tex_labels;
		field->getter = [=]() {return from->GetTexOverride(rownum); };
		field->setter = [=](int val) {
			from->SetTexOverride(this->rownum, val);
			root->Refresh();
			};

		this->inspectorData->fields.push_back(field);
	}
}
