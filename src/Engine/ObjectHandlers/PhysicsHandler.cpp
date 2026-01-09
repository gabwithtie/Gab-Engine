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
	if (dt == 0)
		return;

	this->localpipeline->Tick(dt);

	for (auto& pair : this->object_list) {

		auto po = pair.second;

		auto ro = dynamic_cast<RigidObject*>(po);

		if (ro == nullptr)
			continue;

		Vector3 newpos;
		Quaternion newrot;

		ro->Get_data()->PullTransformationData(newpos, newrot);
		if (!newpos.isfinite())
		{
			std::cerr << "NAN physics transform, skipping update." << std::endl;
			continue;
		}
		ro->World().position.Set(newpos);
		ro->World().rotation.Set(newrot);

		for (auto& fvpair : this->forcevolume_handler.object_list)
		{
			auto fv = fvpair.second;

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