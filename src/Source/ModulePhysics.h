#pragma once

#include "Module.h"
#include "Globals.h"

// TODO 1: Include Box 2 header
#include "box2d/box2d.h"

// Gravity in m/s^2. Screen Y grows DOWNWARDS and we convert meters <-> pixels without flipping Y,
// so a POSITIVE Y gravity makes bodies fall down on screen. That is why the world receives -GRAVITY_Y.
#define GRAVITY_X 0.0f
#define GRAVITY_Y -7.0f

// Units: Box2D works in meters (float), raylib draws in pixels (int)
#define PIXEL_PER_METER 50.0f // if touched change METER_PER_PIXEL too
#define METER_PER_PIXEL 0.02f  // this is 1 / PIXELS_PER_METER !

#define METER_TO_PIXEL(m) ((int) floor(PIXEL_PER_METER * (m)))
#define PIXEL_TO_METER(p)  ((float) METER_PER_PIXEL * (p))

// Module --------------------------------------
class ModulePhysics : public Module
{
public:
	ModulePhysics(Application* app, bool start_enabled = true);
	~ModulePhysics();

	bool Start();
	update_status PreUpdate();
	update_status PostUpdate();
	bool CleanUp();

	void CreatePinball(b2World* world);

private:

	bool debug;
	b2World* world;

	b2Body* ground;
	b2MouseJoint* mouse_joint;

	b2RevoluteJoint* bumperJoint1;
	b2RevoluteJoint* bumperJoint2;

	b2PrismaticJoint* shooterSpringJoint;
};
