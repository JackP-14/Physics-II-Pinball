#include "Globals.h"
#include "Application.h"
#include "ModuleRender.h"
#include "ModulePhysics.h"

#include "p2Point.h"

#include <math.h>

// Flippers
#define FLIPPER_MOTOR_SPEED 20.0f     // rad/s
#define FLIPPER_MOTOR_TORQUE 1500.0f  // N*m
#define FLIPPER_REST_ANGLE 31.0f      // degrees below the horizontal (as the coffin sprite is drawn)
#define FLIPPER_SWING_ANGLE 60.0f     // degrees it travels when pressed

// Kicker
#define KICKER_PULL_SPEED 1.5f        // m/s while the key is held
#define KICKER_LAUNCH_SPEED 20.0f     // m/s at full pull
#define KICKER_MOTOR_FORCE 2000.0f    // N

ModulePhysics::ModulePhysics(Application* app, bool start_enabled) : Module(app, start_enabled)
{
	world = NULL;
	ground = NULL;
	mouse_joint = NULL;
	debug = false;
	time_accumulator = 0.0f;
}

// Destructor
ModulePhysics::~ModulePhysics()
{
}

bool ModulePhysics::Start()
{
	LOG("Creating Physics 2D environment");

	world = new b2World(b2Vec2(GRAVITY_X, GRAVITY_Y));

	world->SetContinuousPhysics(true); // Enables TOI in the world

	world->SetContactListener(this); // Enables contact functions inside the world

	// Static body used as anchor for every joint (mouse, flippers, kicker)
	b2BodyDef bd;
	ground = world->CreateBody(&bd);

	return true;
}

update_status ModulePhysics::PreUpdate()
{
	// Fixed timestep: we consume the real elapsed time in constant steps,
	// so the simulation runs at the same speed whatever the target FPS is
	float frame_time = GetFrameTime();
	if (frame_time > PHYSICS_MAX_FRAME_TIME)
		frame_time = PHYSICS_MAX_FRAME_TIME;

	time_accumulator += frame_time;

	while (time_accumulator >= PHYSICS_TIMESTEP)
	{
		ApplyVerticalDamping();
		world->Step(PHYSICS_TIMESTEP, 8, 3);
		time_accumulator -= PHYSICS_TIMESTEP;
	}

	return UPDATE_CONTINUE;
}

void ModulePhysics::ApplyVerticalDamping()
{
	// Same formula Box2D uses for its linear damping, v *= 1 / (1 + h * c), but only on Y
	for (b2Body* b = world->GetBodyList(); b; b = b->GetNext())
	{
		PhysBody* pbody = (PhysBody*)b->GetUserData().pointer;
		if (pbody == NULL || pbody->vertical_damping <= 0.0f)
			continue;

		b2Vec2 velocity = b->GetLinearVelocity();
		velocity.y *= 1.0f / (1.0f + PHYSICS_TIMESTEP * pbody->vertical_damping);
		b->SetLinearVelocity(velocity);
	}
}

update_status ModulePhysics::PostUpdate()
{
	if (IsKeyPressed(KEY_F1))
	{
		debug = !debug;
	}

	if (!debug)
	{
		if (mouse_joint != NULL)
		{
			world->DestroyJoint(mouse_joint);
			mouse_joint = NULL;
		}
		return UPDATE_CONTINUE;
	}

	DrawDebug();
	UpdateMouseJoint();

	return UPDATE_CONTINUE;
}

// Called before quitting
bool ModulePhysics::CleanUp()
{
	LOG("Destroying physics world");

	// Free the wrappers of any body still alive (Box2D only frees its own data)
	for (b2Body* b = world->GetBodyList(); b; b = b->GetNext())
	{
		delete (PhysBody*)b->GetUserData().pointer;
	}

	// Delete the whole physics world! (this also frees every body, fixture and joint it owns)
	delete world;
	world = NULL;
	ground = NULL;
	mouse_joint = NULL;

	return true;
}

void ModulePhysics::DrawDebug()
{
	for (b2Body* b = world->GetBodyList(); b; b = b->GetNext())
	{
		for (b2Fixture* f = b->GetFixtureList(); f; f = f->GetNext())
		{
			switch (f->GetType())
			{
				// Draw circles ------------------------------------------------
			case b2Shape::e_circle:
			{
				b2CircleShape* shape = (b2CircleShape*)f->GetShape();
				b2Vec2 pos = b->GetWorldPoint(shape->m_p);

				DrawCircleLines(METER_TO_PIXEL(pos.x), METER_TO_PIXEL(pos.y), (float)METER_TO_PIXEL(shape->m_radius), f->IsSensor() ? YELLOW : MAGENTA);
			}
			break;

			// Draw polygons ------------------------------------------------
			case b2Shape::e_polygon:
			{
				b2PolygonShape* polygonShape = (b2PolygonShape*)f->GetShape();
				int32 count = polygonShape->m_count;
				b2Vec2 prev, v;

				for (int32 i = 0; i < count; ++i)
				{
					v = b->GetWorldPoint(polygonShape->m_vertices[i]);
					if (i > 0)
						DrawLine(METER_TO_PIXEL(prev.x), METER_TO_PIXEL(prev.y), METER_TO_PIXEL(v.x), METER_TO_PIXEL(v.y), f->IsSensor() ? YELLOW : RED);

					prev = v;
				}

				v = b->GetWorldPoint(polygonShape->m_vertices[0]);
				DrawLine(METER_TO_PIXEL(prev.x), METER_TO_PIXEL(prev.y), METER_TO_PIXEL(v.x), METER_TO_PIXEL(v.y), f->IsSensor() ? YELLOW : RED);
			}
			break;

			// Draw chains contour -------------------------------------------
			case b2Shape::e_chain:
			{
				b2ChainShape* shape = (b2ChainShape*)f->GetShape();
				b2Vec2 prev, v;

				for (int32 i = 0; i < shape->m_count; ++i)
				{
					v = b->GetWorldPoint(shape->m_vertices[i]);
					if (i > 0)
						DrawLine(METER_TO_PIXEL(prev.x), METER_TO_PIXEL(prev.y), METER_TO_PIXEL(v.x), METER_TO_PIXEL(v.y), GREEN);
					prev = v;
				}
			}
			break;

			// Draw a single segment(edge) ----------------------------------
			case b2Shape::e_edge:
			{
				b2EdgeShape* shape = (b2EdgeShape*)f->GetShape();
				b2Vec2 v1, v2;

				v1 = b->GetWorldPoint(shape->m_vertex1);
				v2 = b->GetWorldPoint(shape->m_vertex2);
				DrawLine(METER_TO_PIXEL(v1.x), METER_TO_PIXEL(v1.y), METER_TO_PIXEL(v2.x), METER_TO_PIXEL(v2.y), BLUE);
			}
			break;
			}
		}
	}
}

void ModulePhysics::UpdateMouseJoint()
{
	b2Vec2 mouse_position(PIXEL_TO_METER(GetMouseX()), PIXEL_TO_METER(GetMouseY()));

	if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && mouse_joint == NULL)
	{
		b2Body* body_clicked = NULL;

		for (b2Body* b = world->GetBodyList(); b && body_clicked == NULL; b = b->GetNext())
		{
			if (b->GetType() != b2_dynamicBody)
				continue;

			for (b2Fixture* f = b->GetFixtureList(); f; f = f->GetNext())
			{
				if (f->TestPoint(mouse_position))
				{
					body_clicked = b;
					break;
				}
			}
		}

		if (body_clicked != NULL)
		{
			b2MouseJointDef def;
			def.bodyA = ground;
			def.bodyB = body_clicked;
			def.target = mouse_position;
			def.maxForce = 1000.0f * body_clicked->GetMass();
			b2LinearStiffness(def.stiffness, def.damping, 5.0f, 0.7f, def.bodyA, def.bodyB);

			mouse_joint = (b2MouseJoint*)world->CreateJoint(&def);
			body_clicked->SetAwake(true);
		}
	}

	if (mouse_joint != NULL && IsMouseButtonDown(MOUSE_BUTTON_LEFT))
	{
		mouse_joint->SetTarget(mouse_position);

		b2Vec2 anchor_a = mouse_joint->GetTarget();  // the mouse
		b2Vec2 anchor_b = mouse_joint->GetAnchorB(); // the point of the body we clicked
		DrawLine(METER_TO_PIXEL(anchor_a.x), METER_TO_PIXEL(anchor_a.y), METER_TO_PIXEL(anchor_b.x), METER_TO_PIXEL(anchor_b.y), RED);
	}

	if (mouse_joint != NULL && IsMouseButtonReleased(MOUSE_BUTTON_LEFT))
	{
		world->DestroyJoint(mouse_joint);
		mouse_joint = NULL;
	}
}

PhysBody* ModulePhysics::CreateCircle(int x, int y, int radius)
{
	PhysBody* pbody = new PhysBody();

	b2BodyDef body;
	body.type = b2_dynamicBody;
	body.bullet = true;
	body.position.Set(PIXEL_TO_METER(x), PIXEL_TO_METER(y));
	body.userData.pointer = reinterpret_cast<uintptr_t>(pbody);

	b2Body* b = world->CreateBody(&body);

	b2CircleShape shape;
	shape.m_radius = PIXEL_TO_METER(radius);

	b2FixtureDef fixture;
	fixture.shape = &shape;
	fixture.density = 1.0f;
	fixture.friction = 0.1f;
	fixture.restitution = 0.3f;
	b->CreateFixture(&fixture);

	pbody->body = b;

	return pbody;
}

PhysBody* ModulePhysics::CreateRectangle(int x, int y, int width, int height)
{
	PhysBody* pbody = new PhysBody();

	b2BodyDef body;
	body.type = b2_dynamicBody;
	body.position.Set(PIXEL_TO_METER(x), PIXEL_TO_METER(y));
	body.userData.pointer = reinterpret_cast<uintptr_t>(pbody);

	b2Body* b = world->CreateBody(&body);

	b2PolygonShape box;
	box.SetAsBox(PIXEL_TO_METER(width) * 0.5f, PIXEL_TO_METER(height) * 0.5f);

	b2FixtureDef fixture;
	fixture.shape = &box;
	fixture.density = 1.0f;
	b->CreateFixture(&fixture);

	pbody->body = b;

	return pbody;
}

PhysBody* ModulePhysics::CreateRectangleSensor(int x, int y, int half_width, int half_height)
{
	PhysBody* pbody = new PhysBody();

	b2BodyDef body;
	body.type = b2_staticBody;
	body.position.Set(PIXEL_TO_METER(x), PIXEL_TO_METER(y));
	body.userData.pointer = reinterpret_cast<uintptr_t>(pbody);

	b2Body* b = world->CreateBody(&body);

	b2PolygonShape box;
	box.SetAsBox(PIXEL_TO_METER(half_width), PIXEL_TO_METER(half_height));

	b2FixtureDef fixture;
	fixture.shape = &box;
	fixture.isSensor = true;
	b->CreateFixture(&fixture);

	pbody->body = b;

	return pbody;
}

PhysBody* ModulePhysics::CreateCircleSensor(int x, int y, int radius)
{
	PhysBody* pbody = new PhysBody();

	b2BodyDef body;
	body.type = b2_staticBody;
	body.position.Set(PIXEL_TO_METER(x), PIXEL_TO_METER(y));
	body.userData.pointer = reinterpret_cast<uintptr_t>(pbody);

	b2Body* b = world->CreateBody(&body);

	b2CircleShape shape;
	shape.m_radius = PIXEL_TO_METER(radius);

	b2FixtureDef fixture;
	fixture.shape = &shape;
	fixture.isSensor = true;
	b->CreateFixture(&fixture);

	pbody->body = b;

	return pbody;
}

PhysBody* ModulePhysics::CreateChain(const int* points, int size, bool loop, float restitution)
{
	PhysBody* pbody = new PhysBody();

	b2BodyDef body;
	body.type = b2_staticBody;
	body.userData.pointer = reinterpret_cast<uintptr_t>(pbody);

	b2Body* b = world->CreateBody(&body);

	// points is a flat list: x0, y0, x1, y1, ...
	int count = size / 2;
	b2Vec2* vertices = new b2Vec2[count];
	for (int i = 0; i < count; ++i)
	{
		vertices[i].Set(PIXEL_TO_METER(points[i * 2]), PIXEL_TO_METER(points[i * 2 + 1]));
	}

	b2ChainShape shape;
	if (loop)
		shape.CreateLoop(vertices, count);
	else
		shape.CreateChain(vertices, count, vertices[0], vertices[count - 1]);

	delete[] vertices; // the shape keeps its own copy

	b2FixtureDef fixture;
	fixture.shape = &shape;
	fixture.friction = 0.0f;
	fixture.restitution = restitution;
	b->CreateFixture(&fixture);

	pbody->body = b;

	return pbody;
}

PhysBody* ModulePhysics::CreateWall(const int* points, int size, float restitution)
{
	PhysBody* pbody = new PhysBody();

	b2BodyDef body;
	body.type = b2_staticBody;
	body.userData.pointer = reinterpret_cast<uintptr_t>(pbody);

	b2Body* b = world->CreateBody(&body);

	b2FixtureDef fixture;
	fixture.friction = 0.0f;
	fixture.restitution = restitution;

	// One two-sided edge per segment: x0, y0, x1, y1, ...
	for (int i = 0; i + 3 < size; i += 2)
	{
		b2EdgeShape edge;
		edge.SetTwoSided(b2Vec2(PIXEL_TO_METER(points[i]), PIXEL_TO_METER(points[i + 1])),
			b2Vec2(PIXEL_TO_METER(points[i + 2]), PIXEL_TO_METER(points[i + 3])));

		fixture.shape = &edge;
		b->CreateFixture(&fixture);
	}

	pbody->body = b;

	return pbody;
}

PhysBody* ModulePhysics::CreateStaticCircle(int x, int y, int radius, float restitution)
{
	PhysBody* pbody = new PhysBody();

	b2BodyDef body;
	body.type = b2_staticBody;
	body.position.Set(PIXEL_TO_METER(x), PIXEL_TO_METER(y));
	body.userData.pointer = reinterpret_cast<uintptr_t>(pbody);

	b2Body* b = world->CreateBody(&body);

	b2CircleShape shape;
	shape.m_radius = PIXEL_TO_METER(radius);

	b2FixtureDef fixture;
	fixture.shape = &shape;
	fixture.restitution = restitution;
	b->CreateFixture(&fixture);

	pbody->body = b;

	return pbody;
}

PhysBody* ModulePhysics::CreateFlipper(int x, int y, int length, bool left)
{
	PhysBody* pbody = new PhysBody();

	// The body origin is the pivot. Box2D angles grow clockwise on screen (Y points down),
	// so the left flipper rests at +REST and the right one (pointing -X) at -REST
	float rest_angle = FLIPPER_REST_ANGLE * DEGTORAD;

	b2BodyDef body;
	body.type = b2_dynamicBody;
	body.bullet = true;
	body.position.Set(PIXEL_TO_METER(x), PIXEL_TO_METER(y));
	body.angle = left ? rest_angle : -rest_angle;
	body.userData.pointer = reinterpret_cast<uintptr_t>(pbody);

	b2Body* b = world->CreateBody(&body);

	// Tapered shape: a thick circle at the pivot, a thin one at the tip and a trapezoid between them
	float direction = left ? 1.0f : -1.0f;
	float len = PIXEL_TO_METER(length);
	float pivot_radius = len * 0.18f;
	float tip_radius = len * 0.08f;

	b2FixtureDef fixture;
	fixture.density = 2.0f;
	fixture.friction = 0.1f;
	fixture.restitution = 0.1f;

	b2CircleShape pivot_shape;
	pivot_shape.m_radius = pivot_radius;
	fixture.shape = &pivot_shape;
	b->CreateFixture(&fixture);

	b2CircleShape tip_shape;
	tip_shape.m_p.Set(direction * len, 0.0f);
	tip_shape.m_radius = tip_radius;
	fixture.shape = &tip_shape;
	b->CreateFixture(&fixture);

	b2Vec2 vertices[4];
	vertices[0].Set(0.0f, -pivot_radius);
	vertices[1].Set(direction * len, -tip_radius);
	vertices[2].Set(direction * len, tip_radius);
	vertices[3].Set(0.0f, pivot_radius);

	b2PolygonShape body_shape;
	body_shape.Set(vertices, 4);
	fixture.shape = &body_shape;
	b->CreateFixture(&fixture);

	// Revolute joint against the ground. Joint angle 0 = rest position
	b2RevoluteJointDef joint;
	joint.Initialize(ground, b, b->GetPosition());
	joint.enableLimit = true;
	if (left)
	{
		joint.lowerAngle = -FLIPPER_SWING_ANGLE * DEGTORAD;
		joint.upperAngle = 0.0f;
	}
	else
	{
		joint.lowerAngle = 0.0f;
		joint.upperAngle = FLIPPER_SWING_ANGLE * DEGTORAD;
	}
	joint.enableMotor = true;
	joint.maxMotorTorque = FLIPPER_MOTOR_TORQUE;
	joint.motorSpeed = 0.0f;

	pbody->body = b;
	pbody->joint = world->CreateJoint(&joint);

	SetFlipperActive(pbody, false);

	return pbody;
}

void ModulePhysics::SetFlipperActive(PhysBody* flipper, bool active)
{
	b2RevoluteJoint* joint = (b2RevoluteJoint*)flipper->joint;

	// The left flipper goes up with negative speed, the right one with positive speed
	float up_speed = (joint->GetLowerLimit() < 0.0f) ? -FLIPPER_MOTOR_SPEED : FLIPPER_MOTOR_SPEED;
	joint->SetMotorSpeed(active ? up_speed : -up_speed);
}

PhysBody* ModulePhysics::CreateKicker(int x, int y, int half_width, int max_pull)
{
	PhysBody* pbody = new PhysBody();

	b2BodyDef body;
	body.type = b2_dynamicBody;
	body.position.Set(PIXEL_TO_METER(x), PIXEL_TO_METER(y));
	body.userData.pointer = reinterpret_cast<uintptr_t>(pbody);

	b2Body* b = world->CreateBody(&body);

	b2PolygonShape box;
	box.SetAsBox(PIXEL_TO_METER(half_width), PIXEL_TO_METER(8));

	b2FixtureDef fixture;
	fixture.shape = &box;
	fixture.density = 2.0f;
	fixture.friction = 0.5f;
	fixture.restitution = 0.0f;
	b->CreateFixture(&fixture);

	// Prismatic joint along +Y (down): translation 0 = rest, max_pull = fully pulled
	b2PrismaticJointDef joint;
	joint.Initialize(ground, b, b->GetPosition(), b2Vec2(0.0f, 1.0f));
	joint.enableLimit = true;
	joint.lowerTranslation = 0.0f;
	joint.upperTranslation = PIXEL_TO_METER(max_pull);
	joint.enableMotor = true;
	joint.maxMotorForce = KICKER_MOTOR_FORCE;
	joint.motorSpeed = -KICKER_PULL_SPEED;

	pbody->body = b;
	pbody->joint = world->CreateJoint(&joint);

	return pbody;
}

void ModulePhysics::PullKicker(PhysBody* kicker)
{
	((b2PrismaticJoint*)kicker->joint)->SetMotorSpeed(KICKER_PULL_SPEED);
}

void ModulePhysics::ReleaseKicker(PhysBody* kicker)
{
	// The further it was pulled, the harder it hits
	((b2PrismaticJoint*)kicker->joint)->SetMotorSpeed(-KICKER_LAUNCH_SPEED * GetKickerPull(kicker));
}

float ModulePhysics::GetKickerPull(const PhysBody* kicker) const
{
	b2PrismaticJoint* joint = (b2PrismaticJoint*)kicker->joint;
	float pull = joint->GetJointTranslation() / joint->GetUpperLimit();
	return (pull < 0.0f) ? 0.0f : (pull > 1.0f) ? 1.0f : pull;
}

void ModulePhysics::SetGravity(float gravity_y)
{
	world->SetGravity(b2Vec2(GRAVITY_X, gravity_y));

	// Sleeping bodies would ignore the change until something touches them
	for (b2Body* b = world->GetBodyList(); b; b = b->GetNext())
	{
		if (b->GetType() == b2_dynamicBody)
			b->SetAwake(true);
	}
}

float ModulePhysics::GetGravity() const
{
	return world->GetGravity().y;
}

void ModulePhysics::DestroyBody(PhysBody* pbody)
{
	if (pbody == NULL)
		return;

	// The mouse joint would be destroyed with the body: forget it first
	if (mouse_joint != NULL && mouse_joint->GetBodyB() == pbody->body)
		mouse_joint = NULL;

	world->DestroyBody(pbody->body); // Box2D frees the body, its fixtures and its joints
	delete pbody;                    // we free our own wrapper
}

void ModulePhysics::BeginContact(b2Contact* contact)
{
	PhysBody* a = (PhysBody*)contact->GetFixtureA()->GetBody()->GetUserData().pointer;
	PhysBody* b = (PhysBody*)contact->GetFixtureB()->GetBody()->GetUserData().pointer;

	if (a != NULL && a->listener != NULL)
		a->listener->OnCollision(a, b);

	if (b != NULL && b->listener != NULL)
		b->listener->OnCollision(b, a);
}

void ModulePhysics::EndContact(b2Contact* contact)
{
	PhysBody* a = (PhysBody*)contact->GetFixtureA()->GetBody()->GetUserData().pointer;
	PhysBody* b = (PhysBody*)contact->GetFixtureB()->GetBody()->GetUserData().pointer;

	if (a != NULL && a->listener != NULL)
		a->listener->OnCollisionEnd(a, b);

	if (b != NULL && b->listener != NULL)
		b->listener->OnCollisionEnd(b, a);
}

// PhysBody ------------------------------------
void PhysBody::GetPosition(int& x, int& y) const
{
	b2Vec2 pos = body->GetPosition();
	x = METER_TO_PIXEL(pos.x);
	y = METER_TO_PIXEL(pos.y);
}

void PhysBody::GetLinearVelocity(float& vx, float& vy) const
{
	b2Vec2 velocity = body->GetLinearVelocity();
	vx = velocity.x;
	vy = velocity.y;
}

float PhysBody::GetRotation() const
{
	return body->GetAngle();
}

void PhysBody::SetPosition(int x, int y)
{
	body->SetTransform(b2Vec2(PIXEL_TO_METER(x), PIXEL_TO_METER(y)), body->GetAngle());
	body->SetAwake(true);
}

void PhysBody::SetLinearVelocity(float vx, float vy)
{
	body->SetLinearVelocity(b2Vec2(vx, vy));
	body->SetAngularVelocity(0.0f);
	body->SetAwake(true);
}

void PhysBody::SetGravityScale(float scale)
{
	body->SetGravityScale(scale);
}

void PhysBody::SetVerticalDamping(float damping)
{
	vertical_damping = damping;
	body->SetAwake(true);
}

void PhysBody::SetRestitution(float restitution)
{
	for (b2Fixture* fixture = body->GetFixtureList(); fixture; fixture = fixture->GetNext())
		fixture->SetRestitution(restitution);
}

void PhysBody::SetFriction(float friction)
{
	for (b2Fixture* fixture = body->GetFixtureList(); fixture; fixture = fixture->GetNext())
		fixture->SetFriction(friction);
}

bool PhysBody::Contains(int x, int y) const
{
	b2Vec2 p(PIXEL_TO_METER(x), PIXEL_TO_METER(y));

	// True if the point is inside ANY of the shapes of this body
	for (b2Fixture* fixture = body->GetFixtureList(); fixture; fixture = fixture->GetNext())
	{
		if (fixture->GetShape()->TestPoint(body->GetTransform(), p))
			return true;
	}

	return false;
}

int PhysBody::RayCast(int x1, int y1, int x2, int y2, float& normal_x, float& normal_y) const
{
	// No hit: return -1
	// Hit: fill normal_x and normal_y and return the distance between x1,y1 and the hit point
	int ret = -1;

	b2RayCastInput input;
	input.p1.Set(PIXEL_TO_METER(x1), PIXEL_TO_METER(y1));
	input.p2.Set(PIXEL_TO_METER(x2), PIXEL_TO_METER(y2));
	input.maxFraction = 1.0f;

	float closest = 1.0f;

	for (b2Fixture* fixture = body->GetFixtureList(); fixture; fixture = fixture->GetNext())
	{
		// A chain has one child per edge
		for (int32 i = 0; i < fixture->GetShape()->GetChildCount(); ++i)
		{
			b2RayCastOutput output;
			if (fixture->GetShape()->RayCast(&output, input, body->GetTransform(), i) && output.fraction <= closest)
			{
				closest = output.fraction;

				float dx = (float)(x2 - x1);
				float dy = (float)(y2 - y1);
				ret = (int)(output.fraction * sqrtf(dx * dx + dy * dy));

				normal_x = output.normal.x;
				normal_y = output.normal.y;
			}
		}
	}

	return ret;
}
