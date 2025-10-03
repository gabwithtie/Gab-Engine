#pragma once

#include "Root.h"
#include "Object.h"
#include "GenericObject.h"
#include "ObjectNamer.h"

#include "Rendering/Camera.h"
#include "Rendering/RenderObject.h"
#include "Rendering/Lights/LightObject.h"
#include "Rendering/Lights/DirectionalLight.h"
#include "Rendering/Lights/ConeLight.h"
#include "Rendering/Lights/PointLight.h"

#include "Physics/ForceVolume.h"
#include "Physics/PhysicsObject.h"
#include "Physics/RigidObject.h"
#include "Physics/TriggerRigidObject.h"
#include "Physics/Collider/Collider.h"
#include "Physics/Collider/BoxCollider.h"
#include "Physics/Collider/SphereCollider.h"
#include "Physics/Collider/MeshCollider.h"
#include "Physics/Collider/CapsuleCollider.h"

#include "Input/InputCustomer.h"
#include "Input/InputPlayer.h"

#include "Controllers/ControllerBase.h"
#include "Controllers/FlyingCameraControl.h"
#include "Controllers/GenericController.h"
#include "Controllers/OrbitalControl.h"