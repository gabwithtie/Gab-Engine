#include "PhysicsObject.h"

gbe::PhysicsObject::~PhysicsObject()
{
}

void gbe::PhysicsObject::OnLocalTransformationChange(TransformChangeType type)
{
	Object::OnLocalTransformationChange(type);
	auto const& wmatrix = this->World().GetMatrix(false);

	if(!wmatrix.isfinite()) {
		throw std::runtime_error("Passing NAN matrix to physics.");
		return;
	}

	this->body->InjectCurrentTransformMatrix(this->World().GetMatrix(false));
}

void gbe::PhysicsObject::OnExternalTransformationChange(TransformChangeType type, Matrix4 parentmat)
{
	Object::OnExternalTransformationChange(type, parentmat);

	if (!parentmat.isfinite()) {
		throw std::runtime_error("Passing NAN matrix to physics.");
		return;
	}

	this->body->InjectCurrentTransformMatrix(this->World().GetMatrix(false));
}

void gbe::PhysicsObject::UpdateCollider(Collider* what)
{
	this->body->UpdateColliderTransform(what->GetColliderData());
	this->body->UpdateAABB();
}

void gbe::PhysicsObject::Set_lookup_func(std::function<PhysicsObject* (physics::PhysicsBody*)>* newfunc)
{
	this->lookup_func = newfunc;
}

gbe::physics::PhysicsBody* gbe::PhysicsObject::Get_data()
{
	return this->body;
}

const std::list<gbe::Collider*> gbe::PhysicsObject::Get_colliders() {
	return this->colliders;
}

void gbe::PhysicsObject::On_Change_enabled(bool _to) {
	Object::On_Change_enabled(_to);

	if (_to) {
		this->body->Activate();
	}
	else {
		this->body->Deactivate();
	}
}

void gbe::PhysicsObject::OnEnterHierarchy(Object* newChild)
{
	Object::OnEnterHierarchy(newChild);

	auto col = dynamic_cast<Collider*>(newChild);

	if (col == nullptr)
		return;

	this->colliders.push_back(col);
	this->body->AddCollider(col->GetColliderData());
	col->AssignToBody(this);
}

void gbe::PhysicsObject::OnExitHierarchy(Object* newChild)
{
	Object::OnExitHierarchy(newChild);

	auto col = dynamic_cast<Collider*>(newChild);

	if (col == nullptr)
		return;

	col->UnAssignBody();
	this->body->RemoveCollider(col->GetColliderData());
	this->colliders.remove(col);
}
