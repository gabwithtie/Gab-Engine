#include "PhysicsWorld.h"

#include <bullet/BulletCollision/CollisionDispatch/btGhostObject.h>

#include "PhysicsBody.h"

void gbe::physics::PhysicsWorld::internal_physics_callback(btDynamicsWorld* world, btScalar timeStep)
{
}

bool gbe::physics::PhysicsWorld::Init()
{
	this->collisionConfiguration = new btDefaultCollisionConfiguration();
	///use the default collision dispatcher. For parallel processing you can use a diffent dispatcher (see Extras/BulletMultiThreaded)
	this->dispatcher = new btCollisionDispatcher(collisionConfiguration);
	///btDbvtBroadphase is a good general purpose broadphase. You can also try out btAxis3Sweep.
	this->overlappingPairCache = new btAxisSweep3(PhysicsVector3(-100), PhysicsVector3(100));
	///the default constraint solver. For parallel processing you can use a different solver (see Extras/BulletMultiThreaded)
	this->solver = new btSequentialImpulseConstraintSolver;

	this->dynamicsWorld = new btDiscreteDynamicsWorld(dispatcher, overlappingPairCache, solver, collisionConfiguration);
	this->dynamicsWorld->setGravity(btVector3(0, 0, 0));
	this->dynamicsWorld->getBroadphase()->getOverlappingPairCache()->setInternalGhostPairCallback(new btGhostPairCallback());

	btContactSolverInfo& info = this->dynamicsWorld->getSolverInfo();
	info.m_solverMode |= SOLVER_INTERLEAVE_CONTACT_AND_FRICTION_CONSTRAINTS;

	auto callback = [this](btDynamicsWorld* world, btScalar timeStep) {
		this->OnFixedUpdate_callback(timeStep);
		};
	//this->dynamicsWorld->setInternalTickCallback(callback, this, false);

	return true;
}

void gbe::physics::PhysicsWorld::Tick(double delta)
{
	for (auto& val : this->body_wrapper_dictionary)
	{
		val.second->Pre_Tick_function(delta);
	}

	dynamicsWorld->stepSimulation(delta, 30);
}

btDiscreteDynamicsWorld* gbe::physics::PhysicsWorld::Get_world()
{
	return this->dynamicsWorld;
}

gbe::physics::PhysicsBody* gbe::physics::PhysicsWorld::GetRelatedBody(const btCollisionObject* key) {
	auto enume = this->body_wrapper_dictionary.find(key);

	if (enume == this->body_wrapper_dictionary.end())
		return nullptr;

	return enume->second;
}

gbe::physics::ColliderData* gbe::physics::PhysicsWorld::GetRelatedCollider(const btCollisionShape* key) {
	auto enume = this->collider_wrapper_dictionary.find(key);

	if (enume == this->collider_wrapper_dictionary.end())
		return nullptr;

	return enume->second;
}