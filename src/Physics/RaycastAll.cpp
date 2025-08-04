#include "RaycastAll.h"

#include "PhysicsWorld.h"
#include "PhysicsPipeline.h"
#include "BulletCollision/NarrowPhaseCollision/btRaycastCallback.h"
#include "BulletCollision/Gimpact/btGImpactShape.h"

gbe::physics::RaycastAll::RaycastAll(PhysicsVector3 from, PhysicsVector3 dir)
{
	PhysicsVector3 to = (PhysicsVector3)(from + dir);

	btCollisionWorld::AllHitsRayResultCallback allResults(from, to);
	allResults.m_flags |= btTriangleRaycastCallback::kF_FilterBackfaces;

	auto cur_context = physics::PhysicsPipeline::GetContext();
	cur_context->Get_world()->rayTest(from, to, allResults);

	this->result = allResults.hasHit();
	
	if (this->result) {
		//LOOP AND TURN INTO AN ARRAY

		//this->others = PhysicsWorld::GetRelatedBody(closestResults.m_collisionObject)->Get_wrapper();
		//this->intersection = closestResults.m_hitPointWorld;
		Vector3 delta = from - this->intersection;
		this->distance = delta.Magnitude();
	}
}
