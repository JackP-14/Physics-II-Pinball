#include "Globals.h"
#include "Application.h"
#include "ModuleRender.h"
#include "ModulePhysics.h"

#include "p2Point.h"

#include <math.h>



ModulePhysics::ModulePhysics(Application* app, bool start_enabled) : Module(app, start_enabled)
{
	world = NULL;
	ground = NULL;
	mouse_joint = NULL;
	bumperJoint1 = NULL;
	bumperJoint2 = NULL;
	shooterSpringJoint = NULL;
	debug = true;
}

// Destructor
ModulePhysics::~ModulePhysics()
{
}

bool ModulePhysics::Start()
{
	LOG("Creating Physics 2D environment");

	world = new b2World(b2Vec2(GRAVITY_X, -GRAVITY_Y));

	world->SetContinuousPhysics(true); // Enables TOI in the world

	world->SetContactListener(this); // Enables contact functions inside the world

	b2BodyDef bd;
	ground = world->CreateBody(&bd);

	CreatePinball(world);

	return true;
}

update_status ModulePhysics::PreUpdate()
{
	world->Step(1.0f / 60.0f, 8, 3);

	return UPDATE_CONTINUE;
}

update_status ModulePhysics::PostUpdate()
{
	if (IsKeyPressed(KEY_SPACE))
	{
		CreateBall(GetMouseX(), GetMouseY(), 10);
	}

	if (IsKeyDown(KEY_A))
	{
		bumperJoint1->SetMotorSpeed(-60.0f);
	}
	else
	{
		bumperJoint1->SetMotorSpeed(60.0f);
	}

	if (IsKeyDown(KEY_D))
	{
		bumperJoint2->SetMotorSpeed(60.0f);
	}
	else
	{
		bumperJoint2->SetMotorSpeed(-60.0f);
	}

	if (IsKeyDown(KEY_S))
	{
		shooterSpringJoint->SetMotorSpeed(3.0f);
	}
	else
	{
		shooterSpringJoint->SetMotorSpeed(-60.0f);
	}

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

	b2Vec2 mouse_position(PIXEL_TO_METER(GetMouseX()), PIXEL_TO_METER(GetMouseY()));
	b2Body* body_clicked = NULL;

	// Bonus code: this will iterate all objects in the world and draw the circles
	for (b2Body* b = world->GetBodyList(); b; b = b->GetNext())
	{
		for(b2Fixture* f = b->GetFixtureList(); f; f = f->GetNext())
		{
			switch(f->GetType())
			{
				// Draw circles ------------------------------------------------
				case b2Shape::e_circle:
				{
					b2CircleShape* shape = (b2CircleShape*)f->GetShape();
					b2Vec2 pos = f->GetBody()->GetPosition();
					
					DrawCircle(METER_TO_PIXEL(pos.x), METER_TO_PIXEL(pos.y), (float)METER_TO_PIXEL(shape->m_radius), Color{0, 0, 0, 128});
				}
				break;

				// Draw polygons ------------------------------------------------
				case b2Shape::e_polygon:
				{
					b2PolygonShape* polygonShape = (b2PolygonShape*)f->GetShape();
					int32 count = polygonShape->m_count;
					b2Vec2 prev, v;

					for(int32 i = 0; i < count; ++i)
					{
						v = b->GetWorldPoint(polygonShape->m_vertices[i]);
						if(i > 0)
							DrawLine(METER_TO_PIXEL(prev.x), METER_TO_PIXEL(prev.y), METER_TO_PIXEL(v.x), METER_TO_PIXEL(v.y), RED);

						prev = v;
					}

					v = b->GetWorldPoint(polygonShape->m_vertices[0]);
					DrawLine(METER_TO_PIXEL(prev.x), METER_TO_PIXEL(prev.y), METER_TO_PIXEL(v.x), METER_TO_PIXEL(v.y), RED);
				}
				break;

				// Draw chains contour -------------------------------------------
				case b2Shape::e_chain:
				{
					b2ChainShape* shape = (b2ChainShape*)f->GetShape();
					b2Vec2 prev, v;

					for(int32 i = 0; i < shape->m_count; ++i)
					{
						v = b->GetWorldPoint(shape->m_vertices[i]);
						if(i > 0)
							DrawLine(METER_TO_PIXEL(prev.x), METER_TO_PIXEL(prev.y), METER_TO_PIXEL(v.x), METER_TO_PIXEL(v.y), GREEN);
						prev = v;
					}

					v = b->GetWorldPoint(shape->m_vertices[0]);
					DrawLine(METER_TO_PIXEL(prev.x), METER_TO_PIXEL(prev.y), METER_TO_PIXEL(v.x), METER_TO_PIXEL(v.y), GREEN);
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

			if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && mouse_joint == NULL && body_clicked == NULL
				&& b->GetType() == b2_dynamicBody && f->TestPoint(mouse_position))
			{
				body_clicked = b;
			}
		}
	}

	if (body_clicked != NULL)
	{
		b2MouseJointDef def;
		def.bodyA = ground;
		def.bodyB = body_clicked;
		def.target = mouse_position;
		def.maxForce = 100000000000.0f * body_clicked->GetMass(); // Max force of the mouse-dragged object
		b2LinearStiffness(def.stiffness, def.damping, 20.0f, 1.0f, def.bodyA, def.bodyB);

		mouse_joint = (b2MouseJoint*)world->CreateJoint(&def);
		body_clicked->SetAwake(true);
	}

	if (mouse_joint != NULL && IsMouseButtonDown(MOUSE_BUTTON_LEFT))
	{
		mouse_joint->SetTarget(mouse_position);

		b2Vec2 anchor_a = mouse_joint->GetAnchorA(); // the target
		b2Vec2 anchor_b = mouse_joint->GetAnchorB(); // the point of the body we clicked
		DrawLine(METER_TO_PIXEL(anchor_a.x), METER_TO_PIXEL(anchor_a.y), METER_TO_PIXEL(anchor_b.x), METER_TO_PIXEL(anchor_b.y), RED);
	}

	if (mouse_joint != NULL && IsMouseButtonReleased(MOUSE_BUTTON_LEFT))
	{
		world->DestroyJoint(mouse_joint);
		mouse_joint = NULL;
	}


	return UPDATE_CONTINUE;
}


// Called before quitting
bool ModulePhysics::CleanUp()
{
	LOG("Destroying physics world");

	// Delete the whole physics world! (this also frees every body, fixture and joint it owns)
	delete world;
	world = NULL;

	return true;
}

void ModulePhysics::CreatePinball(b2World* world)
{
	// Spawn main Pinball body
	b2BodyDef pinballBody;
	pinballBody.type = b2_staticBody;

	//pinballBody.position.Set(SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2);
	pinballBody.position.Set(PIXEL_TO_METER(GetScreenWidth() / 3), PIXEL_TO_METER(GetScreenHeight() / 10));

	b2Body* p = world->CreateBody(&pinballBody);

	b2Vec2 vertices[4];
	vertices[0].Set(0.0f, 0.0f);
	vertices[1].Set(0.0f, PIXEL_TO_METER(GetScreenHeight() / 1.2));
	vertices[2].Set(PIXEL_TO_METER(GetScreenWidth() / 3), PIXEL_TO_METER(GetScreenHeight() / 1.2));
	vertices[3].Set(PIXEL_TO_METER(GetScreenWidth() / 3), 0.0f);

	b2ChainShape pinballShape;
	pinballShape.CreateLoop(vertices, 4);

	b2FixtureDef pinballFixture;
	pinballFixture.shape = &pinballShape;
	pinballFixture.density = 1.0f;
	pinballFixture.friction = 0.0f;
	pinballFixture.restitution = 0.2f;
	p->CreateFixture(&pinballFixture);

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	b2BodyDef esquinaBody;
	esquinaBody.type = b2_staticBody;

	esquinaBody.position.Set(pinballBody.position.x * 2 - PIXEL_TO_METER(125), pinballBody.position.y);

	b2Body* eB = world->CreateBody(&esquinaBody);

	b2Vec2 verticesEsquina[3];
	verticesEsquina[0].Set(PIXEL_TO_METER(0.0f),   PIXEL_TO_METER(0.0f));
	verticesEsquina[1].Set(PIXEL_TO_METER(125.0f), PIXEL_TO_METER(0.0f));
	verticesEsquina[2].Set(PIXEL_TO_METER(125.0f), PIXEL_TO_METER(125.0f));

	b2PolygonShape esquinaShape;
	esquinaShape.Set(verticesEsquina, 3);

	b2FixtureDef esquinaFixture;
	esquinaFixture.shape = &esquinaShape;
	esquinaFixture.density = 1.0f;
	esquinaFixture.friction = 0.0f;
	esquinaFixture.restitution = 0.2f;
	eB->CreateFixture(&esquinaFixture);

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	b2BodyDef bumperBody;
	bumperBody.type = b2_dynamicBody;

	bumperBody.position.Set(PIXEL_TO_METER(525), PIXEL_TO_METER(550));

	b2Body* bumper = world->CreateBody(&bumperBody);

	b2Vec2 verticesBumper[3];
	verticesBumper[0].Set(PIXEL_TO_METER(-6.25f),  PIXEL_TO_METER(10.825f));
	verticesBumper[1].Set(PIXEL_TO_METER(6.25f),   PIXEL_TO_METER(-10.825f));
	verticesBumper[2].Set(PIXEL_TO_METER(64.952f), PIXEL_TO_METER(37.5f));    

	b2PolygonShape bumperShape;
	bumperShape.Set(verticesBumper, 3);

	b2FixtureDef bumperFixture;
	bumperFixture.shape = &bumperShape;
	bumperFixture.density = 1.0f;
	bumperFixture.friction = 0.0f;
	bumperFixture.restitution = 1.0f;
	bumper->CreateFixture(&bumperFixture);

	b2BodyDef anchorDef1;
	anchorDef1.type = b2_kinematicBody;

	anchorDef1.position.Set(bumperBody.position.x, bumperBody.position.y);

	b2Body* anchor1 = world->CreateBody(&anchorDef1);

	b2CircleShape anchorShape;
	anchorShape.m_radius = PIXEL_TO_METER(1.0f);

	b2FixtureDef anchorFixture;
	anchorFixture.shape = &anchorShape;
	anchorFixture.density = 0.0f;   
	anchorFixture.restitution = 0.0f;
	anchor1->CreateFixture(&anchorFixture);

	b2RevoluteJointDef revoluteJointBumper;
	revoluteJointBumper.Initialize(anchor1, bumper, anchor1->GetWorldCenter());

	revoluteJointBumper.enableLimit = true;
	revoluteJointBumper.lowerAngle = -60 * b2_pi / 180;
	revoluteJointBumper.upperAngle = 0 * b2_pi / 180;

	revoluteJointBumper.enableMotor = true;
	revoluteJointBumper.maxMotorTorque = 1000.0f;
	revoluteJointBumper.motorSpeed = 0.0f;

	bumperJoint1 = (b2RevoluteJoint*)world->CreateJoint(&revoluteJointBumper);

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	b2BodyDef bumperBody2;
	bumperBody2.type = b2_dynamicBody;
	bumperBody2.position.Set(PIXEL_TO_METER(750), PIXEL_TO_METER(550));  

	b2Body* bumper2 = world->CreateBody(&bumperBody2);

	b2Vec2 verticesBumper2[3];
	verticesBumper2[0].Set(PIXEL_TO_METER(6.25f),    PIXEL_TO_METER(10.825f));
	verticesBumper2[1].Set(PIXEL_TO_METER(-6.25f),   PIXEL_TO_METER(-10.825f));
	verticesBumper2[2].Set(PIXEL_TO_METER(-64.952f), PIXEL_TO_METER(37.5f)); 

	b2PolygonShape bumperShape2;
	bumperShape2.Set(verticesBumper2, 3);

	b2FixtureDef bumperFixture2;
	bumperFixture2.shape = &bumperShape2;
	bumperFixture2.density = 1.0f;
	bumperFixture2.friction = 0.0f;
	bumperFixture2.restitution = 1.0f;
	bumper2->CreateFixture(&bumperFixture2);

	b2BodyDef anchorDef2;
	anchorDef2.type = b2_kinematicBody;
	anchorDef2.position.Set(bumperBody2.position.x, bumperBody2.position.y);
	b2Body* anchor2 = world->CreateBody(&anchorDef2);

	b2CircleShape anchorShape2;
	anchorShape2.m_radius = PIXEL_TO_METER(1.0f);

	b2FixtureDef anchorFixture2;
	anchorFixture2.shape = &anchorShape2;
	anchorFixture2.density = 0.0f;
	anchorFixture2.restitution = 0.0f;
	anchor2->CreateFixture(&anchorFixture2);

	b2RevoluteJointDef revoluteJointBumper2;
	revoluteJointBumper2.Initialize(anchor2, bumper2, anchor2->GetWorldCenter());
	revoluteJointBumper2.enableLimit = true;
	revoluteJointBumper2.lowerAngle = 0 * b2_pi / 180;     // mirrored: 0 to +60
	revoluteJointBumper2.upperAngle = 60 * b2_pi / 180;
	revoluteJointBumper2.enableMotor = true;
	revoluteJointBumper2.maxMotorTorque = 1000.0f;
	revoluteJointBumper2.motorSpeed = 0.0f;

	bumperJoint2 = (b2RevoluteJoint*)world->CreateJoint(&revoluteJointBumper2);

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	b2BodyDef anchorShooterSpringDef;
	anchorShooterSpringDef.type = b2_staticBody;
	anchorShooterSpringDef.position.Set(pinballBody.position.x * 2 - PIXEL_TO_METER(30), pinballBody.position.y + PIXEL_TO_METER(GetScreenHeight() / 1.2) - PIXEL_TO_METER(10.0f));
	b2Body* anchorShooterSpring = world->CreateBody(&anchorShooterSpringDef);

	b2CircleShape anchorShooterSpringShape;
	anchorShooterSpringShape.m_radius = PIXEL_TO_METER(1.0f);

	b2FixtureDef anchorShooterSpringFixture;
	anchorShooterSpringFixture.shape = &anchorShooterSpringShape;
	anchorShooterSpringFixture.density = 0.0f;
	anchorShooterSpringFixture.restitution = 0.0f;
	anchorShooterSpring->CreateFixture(&anchorShooterSpringFixture);


	b2BodyDef shooterSpringDef;
	shooterSpringDef.type = b2_dynamicBody;
	shooterSpringDef.position.Set(anchorShooterSpring->GetPosition().x, pinballBody.position.y + PIXEL_TO_METER(GetScreenHeight() / 1.2) - PIXEL_TO_METER(100.0f));

	b2Body* shooterSpring = world->CreateBody(&shooterSpringDef);

	b2PolygonShape shooterSpringShape;
	shooterSpringShape.SetAsBox(PIXEL_TO_METER(30.0f), PIXEL_TO_METER(7.5f));

	b2FixtureDef shooterSpringFixture;
	shooterSpringFixture.shape = &shooterSpringShape;
	shooterSpringFixture.density = 1.0f;
	shooterSpringFixture.friction = 0.0f;
	shooterSpringFixture.restitution = 1.0f;
	shooterSpring->CreateFixture(&shooterSpringFixture);


	b2PrismaticJointDef shooterSpringJointDef;
	b2Vec2 worldAxis(0.0f, 1.0f);
	shooterSpringJointDef.Initialize(anchorShooterSpring, shooterSpring, anchorShooterSpring->GetWorldCenter(), worldAxis);
	shooterSpringJointDef.lowerTranslation = PIXEL_TO_METER(-20.0f);
	shooterSpringJointDef.upperTranslation = PIXEL_TO_METER(50.0f); 
	shooterSpringJointDef.enableLimit = true;
	shooterSpringJointDef.maxMotorForce = 500.0f;
	shooterSpringJointDef.motorSpeed = 0.0f;
	shooterSpringJointDef.enableMotor = true;

	shooterSpringJoint = (b2PrismaticJoint*)world->CreateJoint(&shooterSpringJointDef);
}

PhysBody* ModulePhysics::CreateBall(int x, int y, int radius)
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
	b->CreateFixture(&fixture);

	pbody->body = b;

	return pbody;
}

void ModulePhysics::BeginContact(b2Contact* contact)
{
	TraceLog(LOG_INFO, "No me toqueeeeeees");

	PhysBody* a = (PhysBody*)contact->GetFixtureA()->GetBody()->GetUserData().pointer;
	PhysBody* b = (PhysBody*)contact->GetFixtureB()->GetBody()->GetUserData().pointer;

	if (a != NULL && a->listener != NULL)
		a->listener->OnCollision(a, b);

	if (b != NULL && b->listener != NULL)
		b->listener->OnCollision(b, a);
}
