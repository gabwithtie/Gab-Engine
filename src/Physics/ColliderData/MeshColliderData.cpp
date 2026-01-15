#include "MeshColliderData.h"

gbe::physics::MeshColliderData::MeshColliderData(Collider* related_engine_wrapper):
	ColliderData(related_engine_wrapper)
{
}

gbe::physics::MeshColliderData::MeshColliderData(std::vector<std::vector<Vector3>> tris, Collider* related_engine_wrapper) :
	ColliderData(related_engine_wrapper)
{
	this->trimesh = new btTriangleMesh();

	for (auto& tri : tris)
	{
		trimesh->addTriangle((PhysicsVector3)tri[0], (PhysicsVector3)tri[1], (PhysicsVector3)tri[2]);
	}

	this->trimeshShape = new btBvhTriangleMeshShape(trimesh, true );
}

void gbe::physics::MeshColliderData::UpdateMesh(std::vector<std::vector<Vector3>> tris)
{
	if (this->trimeshShape != nullptr)
		delete this->trimeshShape;
	if (this->trimesh != nullptr)
		delete this->trimesh;

	this->trimesh = new btTriangleMesh();

	for (auto& tri : tris)
	{
		trimesh->addTriangle((PhysicsVector3)tri[0], (PhysicsVector3)tri[1], (PhysicsVector3)tri[2]);
	}

	this->trimeshShape = new btBvhTriangleMeshShape(trimesh, true);
}

btCollisionShape* gbe::physics::MeshColliderData::GetShape()
{
	return this->trimeshShape;
}
