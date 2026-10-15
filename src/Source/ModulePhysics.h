#pragma once

#include "Module.h"
#include "Globals.h"

#include "box2d/box2d.h"

#include <vector>


// Gravity in m/s^2. Screen Y grows DOWNWARDS and we convert meters <-> pixels without flipping Y,
// so a POSITIVE Y gravity makes bodies fall down on screen. That is why the world receives -GRAVITY_Y.
#define GRAVITY_X 0.0f
#define GRAVITY_Y -7.0f

// Units: Box2D works in meters (float), raylib draws in pixels (int)
#define PIXEL_PER_METER 50.0f // if touched change METER_PER_PIXEL too
#define METER_PER_PIXEL 0.02f  // this is 1 / PIXELS_PER_METER !

#define METER_TO_PIXEL(m) ((int) floor(PIXEL_PER_METER * (m)))
#define PIXEL_TO_METER(p)  ((float) METER_PER_PIXEL * (p))


class PhysBody
{
public:
	PhysBody() : body(NULL), listener(NULL)
	{
	}

	void GetPosition(int& x, int& y) const;
	float GetRotation() const;
	bool Contains(int x, int y) const;
	int RayCast(int x1, int y1, int x2, int y2, float& normal_x, float& normal_y) const;

public:
	b2Body* body;

	Module* listener;
};

// Module --------------------------------------
class ModulePhysics : public Module, public b2ContactListener
{
public:
	ModulePhysics(Application* app, bool start_enabled = true);
	~ModulePhysics();

	bool Start();
	update_status PreUpdate();
	update_status PostUpdate();
	bool CleanUp();

	PhysBody* CreateRectangleSensor(int x, int y, int half_width, int half_height);

	void CreatePinball(b2World* world);

	PhysBody* CreatePinballOuterWall(b2World* world);

	PhysBody* CreatePinballLeftBumper(b2World* world);

	PhysBody* CreatePinballRightBumper(b2World* world);

	PhysBody* CreatePinballShooter(b2World* world);

	PhysBody* CreatePinballEsquina(b2World* world);

	PhysBody* CreateCircle(int x, int y, int radius);

	void DestroyBody(PhysBody* pbody);

	void BeginContact(b2Contact* contact) override;

	void InstantiatePinball(b2World& world);

private:

	struct TeleportRequest
	{
		PhysBody* body;
		b2Vec2 target; // meters
	};

	bool debug;
	b2World* world;

	b2Body* ground;
	b2MouseJoint* mouse_joint;

	b2RevoluteJoint* bumperJoint1;
	b2RevoluteJoint* bumperJoint2;

	b2PrismaticJoint* shooterSpringJoint;

	PhysBody* outerWall;
	PhysBody* leftBumper;
	PhysBody* rightBumper;
	PhysBody* shooter;
	PhysBody* esquina;

	// Portal
	std::vector<TeleportRequest> teleports;
	PhysBody* portalIn;
	b2Vec2 portalOut; // meters

};