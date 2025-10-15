#pragma once

#include "Engine/Objects/Object.h"
#include "Engine/ObjectHandlers/ObjectHandler.h"

namespace gbe{
	class Root : public Object {
	private:
		std::list<Handler*> handlers;
	public:
		inline Root(){
		}

		inline Root(SerializedObject* data) : Object(data, false) {

		}
		~Root();
		void RegisterHandler(Handler* handler);

		inline Object* GetObjectWithId(unsigned int _id) {
			Object* chosen = nullptr;

			this->CallRecursively([&](Object* child) {
				if (child->Get_id() == _id)
					chosen = child;
				});

			return chosen;
		}

		virtual void OnEnterHierarchy(Object* newChild);
		virtual void OnExitHierarchy(Object* newChild);

		template<typename T>
		ObjectHandler<T>* GetHandler() {
			ObjectHandler<T>* toreturn = nullptr;

			for (auto handler : this->handlers)
			{
				toreturn = dynamic_cast<ObjectHandler<T>*>(handler);

				if (toreturn != nullptr)
					break;
			}

			return toreturn;
		}
	};
}