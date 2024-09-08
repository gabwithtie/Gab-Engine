#include "PhysicsHandler.h"

#include <glm/gtx/quaternion.hpp>


gde::PhysicsHandler::PhysicsHandler()
{
	this->subhandlers.push_back(&this->forcevolume_handler);
}

void gde::PhysicsHandler::AddWorkFunction(RigidObject* ro)
{
	std::unique_lock<std::mutex> lock(mtx); // Lock the mutex
	cond_var.wait(lock, [this] { return this->buffer.size() < 10; }); // Wait until there's space in buffer
	buffer.push(ro); // Add value to the buffer
	lock.unlock(); // Unlock the mutex
	cond_var.notify_one(); // Notify one waiting thread
}

void gde::PhysicsHandler::WorkerFunction()
{
	std::unique_lock<std::mutex> lock(mtx); // Lock the mutex
	cond_var.wait(lock, [this] { return this->buffer.size() > 0; }); // Wait until there's something in the buffer
	auto ro = this->buffer.front(); // Get the front value from buffer
	buffer.pop(); // Remove the value from buffer
	lock.unlock(); // Unlock the mutex

	auto& volumes = this->forcevolume_handler.object_list;

	//Translational
	for (auto volume : volumes) {
		volume->TryApply(ro);
	}

	auto total_force = ro->frame_force;

	ro->acceleration = Vector3::zero;
	ro->acceleration += total_force * (1 / ro->mass);

	float dur_f = (float)this->local_duration;

	ro->TranslateWorld((ro->velocity * dur_f) + ((ro->acceleration * (dur_f * dur_f)) * (1.0f / 2.0f)));

	ro->velocity += ro->acceleration * dur_f;
	ro->velocity *= powf(ro->damping, dur_f);

	ro->frame_force = Vector3::zero;

	//Angular
	float mI = ro->MomentOfInertia();
	ro->angularVelocity += ro->accumulatedTorque * this->local_duration * ((float)1 / mI);
	ro->angularVelocity *= powf(ro->amgularDamp, this->local_duration);

	auto aV = ro->angularVelocity * this->local_duration;
	float aVmag = aV.Magnitude();
	Vector3 aVdir = aV * (1.0f / aVmag);

	if (aVmag > 0.0001f) {
		ro->Rotate(aVdir, aVmag);
	}

	ro->accumulatedTorque = Vector3::zero;

	cond_var.notify_one(); // Notify one waiting thread
}

void gde::PhysicsHandler::Update(double duration)
{
	this->local_duration = duration;

	for (auto ro : this->object_list)
		this->AddWorkFunction(ro);
	for (size_t i = 0; i < this->object_list.size(); i++)
		this->WorkerFunction();

	this->GenerateContacts();

	if (this->contacts.size() > 0)
		this->mContactResolver.ResolveContacts(contacts, duration);
}

void gde::PhysicsHandler::AddContact(RigidObject* r1, RigidObject* r2, float restitution, float depth)
{
	CollisionContact* toAdd = new CollisionContact();
	auto delta = (r1->World()->position - r2->World()->position);
	auto delta_mag = delta.Magnitude();

	toAdd->objects[0] = r1;
	toAdd->objects[1] = r2;
	toAdd->restitution = restitution;
	toAdd->contactNormal = delta * (1.0f / delta_mag);
	//toAdd->depth = delta_mag;

	contacts.push_back(toAdd);
}

void gde::PhysicsHandler::AddLink(RigidObjectLink* link)
{
	this->links.push_back(link);
}

void gde::PhysicsHandler::GenerateContacts()
{
	contacts.clear();
	GetOverlap();

	for (auto link : this->links)
	{
		auto contact = link->GetContact();

		if (contact != nullptr)
			contacts.push_back(contact);
	}

}

void gde::PhysicsHandler::GetOverlap()
{
	for (size_t i = 0; i < this->object_list.size() - 1; i++)
	{
		std::list<RigidObject*>::iterator a = std::next(object_list.begin(), i);

		for (size_t j = i + 1; j < this->object_list.size(); j++)
		{
			std::list<RigidObject*>::iterator b = std::next(object_list.begin(), j);

			Vector3 delta = (*a)->World()->position - (*b)->World()->position;
			float rad_delta = (*a)->colliders.front()->GetWorldRadius() + (*b)->colliders.front()->GetWorldRadius();
			float rad2 = rad_delta * rad_delta;
			float delta2 = delta.SqrMagnitude();

			if (delta2 > rad2)
				continue;

			AddContact(*a, *b, 0.9f, sqrt(rad2 - delta2));
		}
	}
}
