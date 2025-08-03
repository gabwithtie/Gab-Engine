#include "PhysicsPipeline.h"

#include <bullet/BulletCollision/CollisionDispatch/btGhostObject.h>

#include "PhysicsBody.h"

gbe::physics::PhysicsPipeline* gbe::physics::PhysicsPipeline::Instance;

void gbe::physics::PhysicsPipeline::internal_physics_callback(btDynamicsWorld* world, btScalar timeStep)
{
}

gbe::physics::PhysicsPipeline* gbe::physics::PhysicsPipeline::Get_Instance() {
	return gbe::physics::PhysicsPipeline::Instance;
}

bool gbe::physics::PhysicsPipeline::Init()
{
	this->Instance = this;

	this->collisionConfiguration = new btDefaultCollisionConfiguration();
	///use the default collision dispatcher. For parallel processing you can use a diffent dispatcher (see Extras/BulletMultiThreaded)
	this->dispatcher = new btCollisionDispatcher(collisionConfiguration);
	///btDbvtBroadphase is a good general purpose broadphase. You can also try out btAxis3Sweep.
	this->overlappingPairCache = new btAxisSweep3(PhysicsVector3(-100), PhysicsVector3(100));
	///the default constraint solver. For parallel processing you can use a different solver (see Extras/BulletMultiThreaded)
	this->solver = new btSequentialImpulseConstraintSolver;

	this->Reset();

	return true;
}

void gbe::physics::PhysicsPipeline::Tick(double delta)
{
	for (auto& val : this->body_wrapper_dictionary)
	{
		val.second->Pre_Tick_function(delta);
	}

	dynamicsWorld->stepSimulation(delta, 30);
}

void gbe::physics::PhysicsPipeline::Reset() {
	this->dynamicsWorld = new btDiscreteDynamicsWorld(dispatcher, overlappingPairCache, solver, collisionConfiguration);
	this->dynamicsWorld->setGravity(btVector3(0, 0, 0));
	this->dynamicsWorld->getBroadphase()->getOverlappingPairCache()->setInternalGhostPairCallback(new btGhostPairCallback());

	btContactSolverInfo& info = this->dynamicsWorld->getSolverInfo();
	info.m_solverMode |= SOLVER_INTERLEAVE_CONTACT_AND_FRICTION_CONSTRAINTS;

	auto callback = [](btDynamicsWorld* world, btScalar timeStep) {
		physics::PhysicsPipeline::Get_Instance()->OnFixedUpdate_callback(timeStep);
		};
	this->dynamicsWorld->setInternalTickCallback(callback, this, false);
}

btDiscreteDynamicsWorld* gbe::physics::PhysicsPipeline::Get_world()
{
	return this->dynamicsWorld;
}

gbe::physics::PhysicsBody* gbe::physics::PhysicsPipeline::GetRelatedBody(const btCollisionObject* key) {
	auto enume = Instance->body_wrapper_dictionary.find(key);

	if (enume == Instance->body_wrapper_dictionary.end())
		return nullptr;

	return enume->second;
}

gbe::physics::ColliderData* gbe::physics::PhysicsPipeline::GetRelatedCollider(const btCollisionShape* key) {
	auto enume = Instance->collider_wrapper_dictionary.find(key);

	if (enume == Instance->collider_wrapper_dictionary.end())
		return nullptr;

	return enume->second;
}