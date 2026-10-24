#pragma once

#include "Module.h"
#include "Globals.h"

#include "box2d/box2d.h"

// Gravity in m/s^2. Screen Y grows DOWNWARDS and we convert meters <-> pixels without flipping Y,
// so a POSITIVE Y gravity makes bodies fall down on screen.
#define GRAVITY_X 0.0f
#define GRAVITY_Y 7.0f

// Units: Box2D works in meters (float), raylib draws in pixels (int)
#define PIXEL_PER_METER 50.0f // if touched change METER_PER_PIXEL too
#define METER_PER_PIXEL 0.02f  // this is 1 / PIXELS_PER_METER !

#define METER_TO_PIXEL(m) ((int) floor(PIXEL_PER_METER * (m)))
#define PIXEL_TO_METER(p)  ((float) METER_PER_PIXEL * (p))

// The simulation always advances in steps of this size, whatever the render FPS is
#define PHYSICS_TIMESTEP (1.0f / 60.0f)
// Max real time simulated in one frame: avoids the "spiral of death" when a frame takes too long
#define PHYSICS_MAX_FRAME_TIME 0.25f

class PhysBody
{
public:
	PhysBody() : body(NULL), joint(NULL), listener(NULL)
	{
	}

	void GetPosition(int& x, int& y) const;
	float GetRotation() const;
	bool Contains(int x, int y) const;
	int RayCast(int x1, int y1, int x2, int y2, float& normal_x, float& normal_y) const;

	// Never call these inside a collision callback (the world is locked during Step)
	void SetPosition(int x, int y);
	void SetLinearVelocity(float vx, float vy); // m/s
	void SetGravityScale(float scale);

	// Applied to every fixture of the body. Existing contacts keep the old value until they end
	void SetRestitution(float restitution);
	void SetFriction(float friction);

public:
	b2Body* body;

	// Joint that drives this body (flippers and kicker), owned by the world
	b2Joint* joint;

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

	// Factories: every PhysBody returned must be released with DestroyBody()
	PhysBody* CreateCircle(int x, int y, int radius);
	PhysBody* CreateRectangle(int x, int y, int width, int height);
	PhysBody* CreateRectangleSensor(int x, int y, int half_width, int half_height);
	// Chains are one-sided: they only collide on the LEFT of the walking direction (on screen).
	// Smooth for rolling: use them for the outer walls and around solid obstacles
	PhysBody* CreateChain(const int* points, int size, bool loop, float restitution = 0.2f);
	// Polyline of two-sided edges: for thin walls the ball can touch from both sides
	PhysBody* CreateWall(const int* points, int size, float restitution = 0.2f);
	// Static circle, e.g. a bumper
	PhysBody* CreateStaticCircle(int x, int y, int radius, float restitution);

	// Flipper rotating around (x, y). Left flippers point right and vice versa
	PhysBody* CreateFlipper(int x, int y, int length, bool left);
	void SetFlipperActive(PhysBody* flipper, bool active);

	// Kicker plate resting at (x, y) that can be pulled down max_pull pixels
	PhysBody* CreateKicker(int x, int y, int half_width, int max_pull);
	void PullKicker(PhysBody* kicker);
	void ReleaseKicker(PhysBody* kicker);
	float GetKickerPull(const PhysBody* kicker) const; // 0..1

	void DestroyBody(PhysBody* pbody);

	void BeginContact(b2Contact* contact) override;

	bool IsDebug() const { return debug; }

	// Vertical gravity in m/s^2 (positive = down on screen)
	void SetGravity(float gravity_y);
	float GetGravity() const;

private:
	void DrawDebug();
	void UpdateMouseJoint();

private:
	bool debug;
	float time_accumulator;

	b2World* world;
	b2Body* ground;
	b2MouseJoint* mouse_joint;
};
