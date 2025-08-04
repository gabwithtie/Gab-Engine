#include "CapsuleCollider.h"

gbe::CapsuleCollider::CapsuleCollider()
{
	this->mData = new physics::CapsuleColliderData(this);
}

gbe::physics::ColliderData* gbe::CapsuleCollider::GetColliderData()
{
	return this->mData;
}

gbe::Object* gbe::CapsuleCollider::Create(SerializedObject data)
{
	return new CapsuleCollider();
}
