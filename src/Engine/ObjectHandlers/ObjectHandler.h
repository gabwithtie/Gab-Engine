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
		std::list<Handler*> subhandlers;
	public:
		std::list<TValue*> t_object_list;
		std::list<Object*> object_list;

		virtual void OnAdd(TValue*) {}
		virtual void OnRemove(TValue*) {}

		virtual void Remove(Object* object) {
			for (auto subhandler : this->subhandlers)
				subhandler->Remove(object);

			auto it = object_list.begin();
			auto it_t = t_object_list.begin();
			while (it != object_list.end()) {
				if (*it == object) {
					it = object_list.erase(it);
					it_t = t_object_list.erase(it_t);
					break;
				}

				++it;
				++it_t;
			}
		}

		virtual bool TryAdd(Object* object) {
			for (auto subhandler : this->subhandlers)
				subhandler->TryAdd(object);

			TValue* typed_object = dynamic_cast<TValue*>(object);

			if (typed_object == nullptr)
				return false;

			for (auto existing : this->t_object_list)
				if (existing == typed_object)
					return false;

			t_object_list.push_back(typed_object);
			object_list.push_back(object);
			OnAdd(typed_object);

			return true;
		}

		void DoOnEnabled(std::function<void(TValue*)> action) {
			auto _it = object_list.begin();
			auto _it_t = t_object_list.begin();
			while (_it != object_list.end()) {
				if ((*_it)->Get_enabled()) {
					action(*_it_t);
				}
				++_it;
				++_it_t;
			}
		}
	};
}