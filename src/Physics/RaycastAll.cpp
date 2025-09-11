#include "RaycastAll.h"

#include "PhysicsWorld.h"
#include "PhysicsPipeline.h"
#include "BulletCollision/NarrowPhaseCollision/btRaycastCallback.h"
#include "BulletCollision/Gimpact/btGImpactShape.h"

gbe::physics::RaycastAll::RaycastAll(PhysicsVector3 from, PhysicsVector3 dir)
{
	PhysicsVector3 to = (PhysicsVector3)(from + dir);

	btCollisionWorld::AllHitsRayResultCallback results(from, to);
	results.m_flags |= btTriangleRaycastCallback::kF_FilterBackfaces;

	auto cur_context = physics::PhysicsPipeline::GetContext();
	cur_context->Get_world()->rayTest(from, to, results);

	this->result = results.hasHit();
	
	if (this->result) {
		//LOOP AND TURN INTO AN ARRAY

		for (size_t i = 0; i < results.m_collisionObjects.size(); i++)
		{
			this->others.push_back(cur_context->GetRelatedBody(results.m_collisionObjects[i])->Get_wrapper());
		}

		Vector3 delta = from - this->intersection;
		this->distance = delta.Magnitude();
	}
}
