#pragma once

#include <list>
#include <vector>
#include <functional>
#include "Engine/Objects/Object.h"

namespace gbe {
	class Handler {
	public:
		virtual void Remove(Object* object) = 0;
		virtual bool TryAdd(Object* object) = 0;
	};

	template<class TValue>
	class ObjectHandler : public Handler {
	protected:
		std::vector<Handler*> subhandlers;
	public:
		std::unordered_map<Object*, TValue*> object_list;

		virtual void OnAdd(TValue*) {}
		virtual void OnRemove(TValue*) {}

		virtual void Remove(Object* object) {
			for (auto subhandler : this->subhandlers)
				subhandler->Remove(object);

			object_list.erase(object);
		}

		virtual bool TryAdd(Object* object) {
			for (auto subhandler : this->subhandlers)
				subhandler->TryAdd(object);

			TValue* typed_object = dynamic_cast<TValue*>(object);

			if (typed_object == nullptr)
				return false;

			auto it = object_list.find(object);

			if(it != object_list.end())
				return false;

			object_list.insert_or_assign(object, typed_object);
			OnAdd(typed_object);

			return true;
		}

		void DoOnEnabled(std::function<void(TValue*)> action) {
			for (auto& existing : this->object_list)
			{
				if(!existing.first->Get_enabled())
					continue;

				action(existing.second);
			}
		}
	};
}