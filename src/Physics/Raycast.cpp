#include "Raycast.h"

#include "PhysicsWorld.h"
#include "PhysicsPipeline.h"
#include "BulletCollision/NarrowPhaseCollision/btRaycastCallback.h"
#include "BulletCollision/Gimpact/btGImpactShape.h"

gbe::physics::Raycast::RaycastCallback::RaycastCallback(PhysicsVector3 from, PhysicsVector3 to) : btCollisionWorld::ClosestRayResultCallback(from, to), hitObject(nullptr), localShapeInfo(nullptr) {}

btScalar gbe::physics::Raycast::RaycastCallback::addSingleResult(btCollisionWorld::LocalRayResult& rayResult, bool normalInWorldSpace) {
    hitObject = rayResult.m_collisionObject;
    localShapeInfo = rayResult.m_localShapeInfo;

    // Access shape information using localShapeInfo and hitObject
    if (localShapeInfo) {
        // Example: If the object is a compound shape
        if (hitObject->getCollisionShape()->getShapeType() == COMPOUND_SHAPE_PROXYTYPE) {
            // Cast to btCompoundShape
            const btCompoundShape* compoundShape = static_cast<const btCompoundShape*>(hitObject->getCollisionShape());

            // Access child shape using localShapeInfo->m_shapePart
            int childShapeIndex = localShapeInfo->m_shapePart;
			if (childShapeIndex < 0)
				childShapeIndex = 0;

            this->hitShape = compoundShape->getChildShape(childShapeIndex);
		}
    }

    return btCollisionWorld::ClosestRayResultCallback::addSingleResult(rayResult, normalInWorldSpace);
}

gbe::physics::Raycast::Raycast(PhysicsVector3 from, PhysicsVector3 dir)
{
	PhysicsVector3 to = (PhysicsVector3)(from + dir);

	RaycastCallback results(from, to);
	results.m_flags |= btTriangleRaycastCallback::kF_FilterBackfaces;

	auto cur_context = physics::PhysicsPipeline::GetContext();
	cur_context->Get_world()->rayTest(from, to, results);
	this->result = results.hasHit();
	
	if (this->result) {
		auto relatedbody = cur_context->GetRelatedBody(results.m_collisionObject);
		auto relatedcollider = cur_context->GetRelatedCollider(results.hitShape);
		this->other = relatedbody->Get_wrapper();
		this->collider = relatedcollider->Get_wrapper();
		this->intersection = results.m_hitPointWorld;
		Vector3 delta = from - this->intersection;
		this->normal = results.m_hitNormalWorld;
		this->distance = delta.Magnitude();
	}
}
