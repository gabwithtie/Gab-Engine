#include "PhysicsHandler.h"
#include "ColliderHandler.h"

#include <iostream>

gbe::PhysicsHandler::PhysicsHandler()
{
	this->localpipeline = new physics::PhysicsWorld();
	this->localpipeline->Init();
	this->localpipeline->Set_OnFixedUpdate_callback(
		[=](float physicsdeltatime) {
			//TODO: Update physics updatables
		}
	);

	//COLLIDER SUBHANDLER
	this->subhandlers.push_back(new ColliderHandler(this->localpipeline));

	//LOOKUP FUNC
	auto map_ptr = &this->map;
	this->lookup_func = [=](physics::PhysicsBody* key) {
		auto it = map_ptr->find(key);
		PhysicsObject* toreturn = nullptr;

		if (it != this->map.end())
			toreturn = it->second;

		return toreturn;
	};

	this->subhandlers.push_back(&this->forcevolume_handler);
}

void gbe::PhysicsHandler::Update(double dt)
{
	this->localpipeline->Tick(dt);

	for (auto po : this->t_object_list) {

		auto ro = dynamic_cast<RigidObject*>(po);

		if (ro == nullptr)
			continue;

		Vector3 newpos;
		Quaternion newrot;

		ro->Get_data()->PassTransformationData(newpos, newrot);
		if (!newpos.isfinite())
		{
			std::cerr << "NAN physics transform, skipping update." << std::endl;
			continue;
		}
		ro->Local().position.Set(newpos);
		ro->Local().rotation.Set(newrot);

		for (auto fv : this->forcevolume_handler.t_object_list)
		{
			fv->TryApply(ro);
		}
	}
}

void gbe::PhysicsHandler::OnAdd(PhysicsObject* ro)
{
	map.insert_or_assign(ro->Get_data(), ro);
	ro->Set_lookup_func(&this->lookup_func);
	this->localpipeline->RegisterBody(ro->Get_data());
}

void gbe::PhysicsHandler::OnRemove(PhysicsObject* ro)
{
	map.erase(ro->Get_data());
	ro->Set_lookup_func(nullptr);
	this->localpipeline->UnRegisterBody(ro->Get_data());
}