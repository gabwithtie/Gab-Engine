#include "MeshCollider.h"

gbe::MeshCollider::MeshCollider(asset::Mesh* mesh):
	mData(this)
{
	UpdateMesh(mesh);
}

void gbe::MeshCollider::UpdateMesh(asset::Mesh* mesh)
{
	if (mesh == nullptr)
		return;

	auto newtris = std::vector<std::vector<Vector3>>();
	auto& verts = mesh->Get_load_data().vertices;

	for (const auto& tri : mesh->Get_load_data().faces)
	{
		if (tri.size() < 3)
			continue;

		newtris.push_back({ verts[tri[0]].pos,verts[tri[1]].pos, verts[tri[2]].pos });
	}

	this->mData = physics::MeshColliderData(newtris, this);
}

void gbe::MeshCollider::UpdateVertices(std::vector<std::vector<Vector3>> verts)
{
	if(this->parent != nullptr)
		this->parent->OnExitHierarchy(this);

	this->mData = physics::MeshColliderData(verts, this);

	if(this->parent != nullptr)
		this->parent->OnEnterHierarchy(this);
}

gbe::physics::ColliderData* gbe::MeshCollider::GetColliderData()
{
	return &this->mData;
}