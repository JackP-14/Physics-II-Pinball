#include "Globals.h"
#include "Application.h"
#include "ModuleRender.h"
#include "ModulePhysics.h"

#include "p2Point.h"

#include <math.h>



ModulePhysics::ModulePhysics(Application* app, bool start_enabled) : Module(app, start_enabled)
{
	world = NULL;
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

	CreatePinball(world);
	

	return true;
}

update_status ModulePhysics::PreUpdate()
{
	world->Step(1.0f / 60.0f, 8, 3);

	return UPDATE_CONTINUE;
}

// 
update_status ModulePhysics::PostUpdate()
{
	if (IsKeyPressed(KEY_SPACE))
	{
		b2BodyDef body;
		body.type = b2_dynamicBody;
		body.position.Set(PIXEL_TO_METER(GetMouseX()), PIXEL_TO_METER(GetMouseY()));

		b2Body* b = world->CreateBody(&body);

		b2CircleShape shape;
		shape.m_radius = PIXEL_TO_METER(25);

		b2FixtureDef fixture;
		fixture.shape = &shape;
		fixture.density = 1.0f;
		b->CreateFixture(&fixture);
	}

	if (IsKeyPressed(KEY_F1))
	{
		debug = !debug;
	}

	if (!debug)
	{
		return UPDATE_CONTINUE;
	}

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
		}
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

	b2BodyDef bumperBody;
	bumperBody.type = b2_staticBody;

	bumperBody.position.Set(PIXEL_TO_METER(450), PIXEL_TO_METER(550));

	b2Body* b = world->CreateBody(&bumperBody);

	b2Vec2 verticesBumper[4];
	verticesBumper[0].Set(PIXEL_TO_METER(0.0f), PIXEL_TO_METER(0.0f));
	verticesBumper[1].Set(PIXEL_TO_METER(129.904f), PIXEL_TO_METER(75.0f));
	verticesBumper[2].Set(PIXEL_TO_METER(104.904f), PIXEL_TO_METER(118.301f));
	verticesBumper[3].Set(PIXEL_TO_METER(-25.0f), PIXEL_TO_METER(43.301f));
	
	b2ChainShape bumperShape;
	bumperShape.CreateLoop(verticesBumper, 4);

	b2FixtureDef bumperFixture;
	bumperFixture.shape = &bumperShape;
	bumperFixture.density = 1.0f;
	bumperFixture.friction = 0.0f;
	bumperFixture.restitution = 1.0f;
	b->CreateFixture(&bumperFixture);

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	b2BodyDef bumperBody2;
	bumperBody2.type = b2_staticBody;

	bumperBody2.position.Set(PIXEL_TO_METER(695), PIXEL_TO_METER(550));

	b2Body* b2Bumper = world->CreateBody(&bumperBody2);

	b2Vec2 verticesBumper2[4];
	verticesBumper2[0].Set(PIXEL_TO_METER(25.0f), PIXEL_TO_METER(118.301f));
	verticesBumper2[1].Set(PIXEL_TO_METER(0.0f), PIXEL_TO_METER(75.0f));
	verticesBumper2[2].Set(PIXEL_TO_METER(129.904f), PIXEL_TO_METER(0.0f));
	verticesBumper2[3].Set(PIXEL_TO_METER(154.904f), PIXEL_TO_METER(43.301f));

	b2ChainShape bumperShape2;
	bumperShape2.CreateLoop(verticesBumper2, 4);

	b2FixtureDef bumperFixture2;
	bumperFixture2.shape = &bumperShape2;
	bumperFixture2.density = 1.0f;
	bumperFixture2.friction = 0.0f;
	bumperFixture2.restitution = 1.0f;
	b2Bumper->CreateFixture(&bumperFixture2);
}
