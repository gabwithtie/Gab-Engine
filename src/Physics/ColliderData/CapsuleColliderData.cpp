#include "CapsuleColliderData.h"

gbe::physics::CapsuleColliderData::CapsuleColliderData(Collider* related_engine_wrapper) : ColliderData(related_engine_wrapper), mShape(1, 2)
{
}

btCollisionShape* gbe::physics::CapsuleColliderData::GetShape()
{
	return &(this->mShape);
}
