#pragma once

#include <list>
#include <queue>
#include "ObjectHandler.h"
#include "../Objects/Physics/Physics.h"
#include "../Objects/Physics/ForceVolume.h"
#include "Members/ContactResolver.h"

#include <thread> // Include for using thread functionality
#include <mutex> // Include for using mutex for synchronization
#include <condition_variable> // Include for using condition variables
#include <atomic>

namespace gde {
	class PhysicsHandler : public ObjectHandler<RigidObject>{
	private:
		double local_duration;

		ObjectHandler<ForceVolume> forcevolume_handler;
		std::vector<CollisionContact*> contacts;

		ContactResolver mContactResolver = ContactResolver(20);
		std::list<RigidObjectLink*> links;

		std::mutex mtx; // Mutex for synchronization
		std::condition_variable cond_var; // Condition variable for producer-consumer signaling
		std::queue<RigidObject*> buffer; // Queue to act as a buffer
		std::list<std::thread> workers;

		void AddWorkFunction(RigidObject* ro);
		void WorkerFunction();
	public:
		PhysicsHandler();

		void Update(double duration);
		void AddContact(RigidObject* r1, RigidObject* r2, float restitution, float depth);
		void AddLink(RigidObjectLink* link);

	protected:
		void GenerateContacts();
		void GetOverlap();
	};
}