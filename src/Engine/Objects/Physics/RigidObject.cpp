#include "RigidObject.h"
#include "RigidObject.h"
#include "RigidObject.h"
#include "RigidObject.h"

gbe::RigidObject::RigidObject(bool _is_static) {
	this->body = new physics::Rigidbody(this, _is_static);

	this->is_static = _is_static;
}

gbe::RigidObject::~RigidObject()
{
}

gbe::physics::Rigidbody* gbe::RigidObject::GetRigidbody()
{
	if (this->body == nullptr)
		throw "Null rigidbody";

	return static_cast<physics::Rigidbody*>(this->body);
}

gbe::Object* gbe::RigidObject::Create(gbe::SerializedObject data) {
	auto _is_static_str = data.serialized_variables["static"];
	bool _is_static = _is_static_str == "1";

	return new RigidObject(_is_static);
}

gbe::SerializedObject gbe::RigidObject::Serialize() {
	auto data = PhysicsObject::Serialize();

	data.serialized_variables.insert_or_assign("static", this->is_static ? "1" : "0");

	return data;
}