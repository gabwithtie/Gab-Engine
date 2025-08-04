#include "TriggerRigidBody.h"
#include <bullet/BulletCollision/CollisionDispatch/btGhostObject.h>
#include "PhysicsWorld.h"

gbe::physics::TriggerRigidBody::TriggerRigidBody(PhysicsObject* object) : PhysicsBody(object) {
	auto newdata = new btGhostObject();

	newdata->setCollisionFlags(btCollisionObject::CF_NO_CONTACT_RESPONSE);

	newdata->setCollisionShape(this->mMainShape);
	newdata->setWorldTransform(this->transform);

	this->base_data = newdata;
}

void gbe::physics::TriggerRigidBody::Pre_Tick_function(float deltatime)
{
}

void gbe::physics::TriggerRigidBody::Activate()
{
	this->world->Get_world()->addCollisionObject(this->base_data);
	PhysicsBody::Activate();
}

void gbe::physics::TriggerRigidBody::Deactivate()
{
	this->world->Get_world()->removeCollisionObject(this->base_data);
	PhysicsBody::Deactivate();
}

int gbe::physics::TriggerRigidBody::Get_numInside() {
	return btGhostObject::upcast(this->base_data)->getNumOverlappingObjects();
}

gbe::physics::PhysicsBody* gbe::physics::TriggerRigidBody::Get_inside(int index) {
	auto key = btGhostObject::upcast(this->base_data)->getOverlappingObject(index);
	return this->world->GetRelatedBody(key);
}

void gbe::physics::TriggerRigidBody::ForceUpdateTransform()
{

}