#include "Globals.h"
#include "Application.h"
#include "ModuleWindow.h"
#include "ModuleRender.h"
#include "ModuleGame.h"
#include "ModuleAudio.h"
#include "ModulePhysics.h"

#include <math.h>

// Table layout, in background image pixels (table_background.png) ---------------
#define BALL_RADIUS 15
#define FLIPPER_LENGTH 68
#define LEFT_FLIPPER_X 232
#define RIGHT_FLIPPER_X 420
#define FLIPPERS_Y 738
#define KICKER_X 584
#define KICKER_Y 699
#define KICKER_HALF_WIDTH 19
#define KICKER_MAX_PULL 35
#define DRAIN_Y 870

#define BUMPER_RADIUS 26
#define BUMPER_RESTITUTION 1.3f   // > 1: bumpers give energy to the ball
#define SLINGSHOT_RESTITUTION 1.4f

// Scoring
#define BUMPER_POINTS 100
#define SLINGSHOT_POINTS 10
#define TARGET_POINTS 250         // first hit of an unlit target
#define TARGET_LIT_POINTS 50      // target already lit
#define PUMPKIN_COMBO_POINTS 1000 // plus an extra ball
#define CANDY_COMBO_POINTS 2000
#define SINKHOLE_POINTS 500

// Sinkhole next to the tombstone: holds the ball and spits it out
#define SINKHOLE_X 126
#define SINKHOLE_Y 428
#define SINKHOLE_HALF_SIZE 8
#define SINKHOLE_HOLD_X 132       // where the ball waits (just off the wall)
#define SINKHOLE_HOLD_Y 436
#define SINKHOLE_HOLD_TIME 1.2f
#define SINKHOLE_COOLDOWN 1.0f
#define SINKHOLE_EJECT_VX 5.0f    // m/s, towards the center of the table
#define SINKHOLE_EJECT_VY 1.0f

#define FLASH_TIME 0.15f
#define MESSAGE_TIME 2.0f

// Runtime physics settings: default, step and range
#define GRAVITY_STEP 1.0f
#define GRAVITY_MIN 1.0f
#define GRAVITY_MAX 20.0f
#define BALL_RESTITUTION_DEFAULT 0.3f
#define BALL_FRICTION_DEFAULT 0.1f
#define MATERIAL_STEP 0.1f
#define MATERIAL_MIN 0.0f
#define MATERIAL_MAX 1.0f

// IMPORTANT: Box2D chains only collide on ONE side: the one on your LEFT when walking
// the points in order on screen. Playfield walls go counter-clockwise around the playfield,
// solid obstacles go clockwise around themselves.

// Outer wall: right wall of the kicker lane, around the top, down the left side to the left inlane
static const int outer_wall[] = {
	607, 750,
	607, 140,
	603, 100,
	588, 62,
	560, 35,
	520, 20,
	460, 12,
	400, 10,
	340, 9,
	280, 7,
	230, 10,
	200, 18,
	180, 30,
	168, 75,
	176, 135,
	198, 205	// top corner of the ramp mouth
};

// Right side of the playfield: right inlane, end of the right ramp, bat, rail behind the cauldron
static const int right_wall[] = {
	430, 722,	// right inlane
	474, 692,
	474, 637,	// behind the right slingshot
	472, 568,
	476, 550,	// end of the right ramp
	482, 528,
	497, 508,
	522, 488,
	545, 464,
	558, 440,
	545, 440,	// bat
	518, 425,
	503, 395,
	506, 360,	// purple rail
	530, 320,
	540, 250,
	535, 205,
	520, 158,
	505, 125,
	512, 112,
	548, 80
};

// Back of the left slingshot: two-sided, so the ball can't get inside the slingshot from the ramp exit
static const int sling_back_wall[] = {
	175, 578,
	177, 640
};

// Guide into the left flipper (one-sided, the ball rolls on top of it)
static const int left_inlane_guide[] = {
	177, 694,
	220, 722
};

// Left ramp, outer edge: exit ring -> left lane -> hairpin -> right strip -> mouth top corner
static const int ramp_outer[] = {
	138, 604,	// exit: open gap between here and (175, 578)
	115, 597,
	92, 578,
	72, 548,
	60, 515,
	58, 480,
	58, 60,
	66, 42,
	78, 30,
	95, 25,
	112, 23,
	130, 26,
	145, 34,
	157, 50,
	165, 75,
	176, 135,
	198, 205
};

// Left ramp, inner edge: closes the slit between the two lanes and follows the candy cards and the tombstone
static const int ramp_inner[] = {
	175, 578,
	158, 562,
	130, 542,
	112, 520,
	105, 480,
	105, 432,
	105, 70,	// left lane, right wall
	107, 63,	// top of the divider
	112, 60,
	117, 62,
	120, 68,
	137, 285,	// right strip, left wall, down to the mouth
	145, 292,	// left candy cards
	123, 362,
	137, 372,	// tombstone
	137, 412,
	120, 418,
	105, 432
};

// Floor under the exit: the top edge of the painted left bar, it feeds the ball into the left inlane
static const int left_inlane_floor[] = {
	88, 628,
	177, 694
};

// Inner edge of the launch ramp and left wall of the kicker lane: the ball touches it from both sides
static const int launch_wall[] = {
	335, 95,
	350, 80,
	400, 58,
	450, 52,
	500, 53,
	530, 62,
	553, 85,
	562, 120,
	562, 750
};

static const int pumpkin[] = {
	225, 150,
	232, 105,
	255, 78,
	290, 64,
	330, 66,
	355, 88,
	366, 130,
	360, 172,
	350, 196,
	240, 196,
	226, 172
};

static const int cauldron[] = {
	440, 282,
	458, 262,
	500, 256,
	540, 262,
	546, 285,
	535, 322,
	500, 342,
	462, 332,
	445, 310
};

static const int candy_bank[] = {
	350, 318,
	365, 292,
	428, 296,
	432, 332,
	430, 356,
	352, 346
};

// Slingshots: the face kicks the ball, the bottom is a normal wall (the back is part of the side walls)
static const int left_sling_face[] = { 180, 567, 227, 672 };
static const int left_sling_bottom[] = { 177, 640, 227, 672 };
static const int right_sling_face[] = { 472, 568, 425, 667 };
static const int right_sling_bottom[] = { 474, 637, 425, 667 };

// Eye bumpers
static const int bumper_positions[BUMPER_COUNT * 2] = {
	397, 108,
	445, 193,
	400, 268
};

// Target sensors (center and half size). Pumpkin: the three slots under the pumpkin
static const int pumpkin_target_positions[TARGET_COUNT * 2] = {
	258, 201,
	295, 201,
	332, 201
};
#define PUMPKIN_TARGET_HALF_W 15
#define PUMPKIN_TARGET_HALF_H 6

// The lights that show the pumpkin progress (the three painted triangles)
static const int pumpkin_light_positions[TARGET_COUNT * 2] = {
	263, 218,
	295, 218,
	327, 218
};

// Candy cards at the bottom of the candy bank
static const int candy_target_positions[TARGET_COUNT * 2] = {
	367, 352,
	391, 355,
	415, 358
};
#define CANDY_TARGET_HALF_W 10
#define CANDY_TARGET_HALF_H 6

// Painted candy cards (x, y, w, h) to highlight them when lit
static const int candy_card_rects[TARGET_COUNT * 4] = {
	355, 320, 23, 32,
	380, 322, 23, 33,
	405, 325, 23, 33
};

#define ARRAY_COUNT(a) ((int)(sizeof(a) / sizeof(a[0])))

// Render target FPS the player can cycle through
static const int fps_options[] = { 15, 30, 60, 120, 144, 240 };
static const int fps_options_count = ARRAY_COUNT(fps_options);

// Flipper sprite: pixel of the pivot and X of the tip in bumpers.png (the sprite points left)
static const Vector2 flipper_sprite_pivot = { 633.0f, 206.0f };
static const float flipper_sprite_tip_x = 20.0f;

// Kicker sprite: the plate inside shooter.png
static const Rectangle kicker_sprite_section = { 65.0f, 80.0f, 305.0f, 105.0f };

// Entities ---------------------------------------------------------------------

// Base class for everything in the scene that has a physics body. Owns that body
class PhysicEntity
{
protected:
	PhysicEntity(ModulePhysics* _physics, PhysBody* _body)
		: physics(_physics)
		, body(_body)
	{
	}

public:
	virtual ~PhysicEntity()
	{
		physics->DestroyBody(body);
	}

	virtual void Draw() const = 0;

public:
	ModulePhysics* physics;
	PhysBody* body;
};

class Ball : public PhysicEntity
{
public:
	Ball(ModulePhysics* physics, int x, int y, int _radius)
		: PhysicEntity(physics, physics->CreateCircle(x, y, _radius))
		, radius(_radius)
	{
	}

	void Draw() const override
	{
		int x, y;
		body->GetPosition(x, y);

		DrawCircle(x, y, (float)radius, Color{ 90, 200, 40, 255 });
		DrawCircle(x - radius / 3, y - radius / 3, radius * 0.35f, Color{ 220, 255, 180, 255 });
		DrawCircleLines(x, y, (float)radius, Color{ 30, 80, 10, 255 });
	}

private:
	int radius;
};

class Flipper : public PhysicEntity
{
public:
	Flipper(ModulePhysics* physics, int x, int y, int _length, bool _left, Texture2D _texture)
		: PhysicEntity(physics, physics->CreateFlipper(x, y, _length, _left))
		, length(_length)
		, left(_left)
		, texture(_texture)
	{
	}

	void SetActive(bool active)
	{
		physics->SetFlipperActive(body, active);
	}

	void Draw() const override
	{
		int x, y;
		body->GetPosition(x, y);

		// Scale so the sprite tip matches the physics tip (plus its rounded end)
		float scale = (length * 1.08f) / (flipper_sprite_pivot.x - flipper_sprite_tip_x);

		// The sprite points left (right flipper): mirror it for the left one
		Rectangle source = { 0.0f, 0.0f, (float)texture.width, (float)texture.height };
		float pivot_x = flipper_sprite_pivot.x;
		if (left)
		{
			source.width = -source.width;
			pivot_x = texture.width - pivot_x;
		}

		Rectangle dest = { (float)x, (float)y, texture.width * scale, texture.height * scale };
		Vector2 origin = { pivot_x * scale, flipper_sprite_pivot.y * scale };
		DrawTexturePro(texture, source, dest, origin, body->GetRotation() * RAD2DEG, WHITE);
	}

private:
	int length;
	bool left;
	Texture2D texture;
};

class Kicker : public PhysicEntity
{
public:
	Kicker(ModulePhysics* physics, int x, int y, int _half_width, int max_pull, Texture2D _texture)
		: PhysicEntity(physics, physics->CreateKicker(x, y, _half_width, max_pull))
		, half_width(_half_width)
		, texture(_texture)
	{
	}

	void Pull()
	{
		physics->PullKicker(body);
	}

	void Release()
	{
		physics->ReleaseKicker(body);
	}

	float GetPull() const
	{
		return physics->GetKickerPull(body);
	}

	void Draw() const override
	{
		int x, y;
		body->GetPosition(x, y);

		float width = half_width * 2.0f;
		float height = width * kicker_sprite_section.height / kicker_sprite_section.width;

		Rectangle dest = { (float)x, (float)y, width, height };
		Vector2 origin = { width / 2.0f, height / 2.0f };
		DrawTexturePro(texture, kicker_sprite_section, dest, origin, 0.0f, WHITE);
	}

private:
	int half_width;
	Texture2D texture;
};

// ModuleGame -------------------------------------------------------------------

ModuleGame::ModuleGame(Application* app, bool start_enabled) : Module(app, start_enabled)
{
	state = GameState::TITLE_SCREEN;
	state_timer = 0.0f;

	score = 0;
	previous_score = 0;
	high_score = 0;
	balls_left = 0;

	for (int i = 0; i < BUMPER_COUNT; ++i)
	{
		bumpers[i] = NULL;
		bumper_flash[i] = 0.0f;
	}
	for (int i = 0; i < SLINGSHOT_COUNT; ++i)
	{
		slingshots[i] = NULL;
		slingshot_flash[i] = 0.0f;
	}
	for (int i = 0; i < TARGET_COUNT; ++i)
	{
		pumpkin_targets.sensors[i] = NULL;
		candy_targets.sensors[i] = NULL;
	}
	pumpkin_targets.Reset();
	candy_targets.Reset();

	drain = NULL;
	sinkhole = NULL;
	left_flipper = NULL;
	right_flipper = NULL;
	kicker = NULL;
	ball = NULL;
	ball_lost = false;
	sinkhole_entered = false;
	sinkhole_timer = 0.0f;
	sinkhole_cooldown = 0.0f;

	message = NULL;
	message_timer = 0.0f;

	ball_restitution = BALL_RESTITUTION_DEFAULT;
	ball_friction = BALL_FRICTION_DEFAULT;

	fx_bumper = fx_slingshot = fx_flipper = fx_kicker = fx_target = 0;
	fx_hole = fx_drain = fx_extra_ball = fx_bonus = fx_round_over = 0;

	table_scale = 1.0f;
	table_offset_x = 0.0f;

	background_tex = Texture2D{ 0 };
	flipper_tex = Texture2D{ 0 };
	kicker_tex = Texture2D{ 0 };
}

int TargetBank::LitCount() const
{
	int count = 0;
	for (int i = 0; i < TARGET_COUNT; ++i)
	{
		if (lit[i])
			++count;
	}
	return count;
}

void TargetBank::Reset()
{
	for (int i = 0; i < TARGET_COUNT; ++i)
		lit[i] = false;
}

ModuleGame::~ModuleGame()
{
}

// Load assets
bool ModuleGame::Start()
{
	LOG("Loading game assets");
	bool ret = true;

	App->renderer->camera.x = App->renderer->camera.y = 0;
	App->renderer->SetBackgroundColor(Color{ 40, 20, 70, 255 });

	background_tex = LoadTexture("Assets/table_background.png");
	flipper_tex = LoadTexture("Assets/bumpers.png");
	kicker_tex = LoadTexture("Assets/shooter.png");

	// The audio module owns the sounds and unloads them
	fx_bumper = App->audio->LoadFx("Assets/Audio/Fx/bumper.wav");
	fx_slingshot = App->audio->LoadFx("Assets/Audio/Fx/slingshot.wav");
	fx_flipper = App->audio->LoadFx("Assets/Audio/Fx/flipper.wav");
	fx_kicker = App->audio->LoadFx("Assets/Audio/Fx/kicker.wav");
	fx_target = App->audio->LoadFx("Assets/Audio/Fx/target.wav");
	fx_hole = App->audio->LoadFx("Assets/Audio/Fx/hole.wav");
	fx_drain = App->audio->LoadFx("Assets/Audio/Fx/drain.wav");
	fx_extra_ball = App->audio->LoadFx("Assets/Audio/Fx/extra_ball.wav");
	fx_round_over = App->audio->LoadFx("Assets/Audio/Fx/round_over.wav");
	fx_bonus = App->audio->LoadFx("Assets/bonus.wav");

	// Fit the table to the screen height, centered horizontally
	table_scale = (float)SCREEN_HEIGHT / background_tex.height;
	table_offset_x = (SCREEN_WIDTH - background_tex.width * table_scale) / 2.0f;

	CreateTable();

	state = GameState::TITLE_SCREEN;

	return ret;
}

// Unload assets
bool ModuleGame::CleanUp()
{
	LOG("Unloading game");

	DestroyBall();
	DestroyTable();

	UnloadTexture(background_tex);
	UnloadTexture(flipper_tex);
	UnloadTexture(kicker_tex);

	return true;
}

int ModuleGame::TableX(float x) const
{
	return (int)(table_offset_x + x * table_scale);
}

int ModuleGame::TableY(float y) const
{
	return (int)(y * table_scale);
}

int ModuleGame::TableLength(float length) const
{
	return (int)(length * table_scale);
}

std::vector<int> ModuleGame::TableToScreen(const int* points, int size) const
{
	std::vector<int> screen_points(size);
	for (int i = 0; i < size; i += 2)
	{
		screen_points[i] = TableX((float)points[i]);
		screen_points[i + 1] = TableY((float)points[i + 1]);
	}
	return screen_points;
}

PhysBody* ModuleGame::CreateTableChain(const int* points, int size, bool loop, float restitution)
{
	std::vector<int> screen_points = TableToScreen(points, size);
	return App->physics->CreateChain(screen_points.data(), size, loop, restitution);
}

PhysBody* ModuleGame::CreateTableWall(const int* points, int size, float restitution)
{
	std::vector<int> screen_points = TableToScreen(points, size);
	return App->physics->CreateWall(screen_points.data(), size, restitution);
}

void ModuleGame::CreateTable()
{
	// Walls
	walls.push_back(CreateTableChain(outer_wall, ARRAY_COUNT(outer_wall), false));
	walls.push_back(CreateTableWall(sling_back_wall, ARRAY_COUNT(sling_back_wall), 0.1f));
	walls.push_back(CreateTableChain(left_inlane_guide, ARRAY_COUNT(left_inlane_guide), false));
	walls.push_back(CreateTableWall(ramp_outer, ARRAY_COUNT(ramp_outer), 0.1f));
	walls.push_back(CreateTableWall(ramp_inner, ARRAY_COUNT(ramp_inner), 0.1f));
	walls.push_back(CreateTableWall(left_inlane_floor, ARRAY_COUNT(left_inlane_floor), 0.1f));
	walls.push_back(CreateTableChain(right_wall, ARRAY_COUNT(right_wall), false));
	walls.push_back(CreateTableWall(launch_wall, ARRAY_COUNT(launch_wall)));
	walls.push_back(CreateTableWall(left_sling_bottom, ARRAY_COUNT(left_sling_bottom)));
	walls.push_back(CreateTableWall(right_sling_bottom, ARRAY_COUNT(right_sling_bottom)));

	// Solid obstacles
	walls.push_back(CreateTableChain(pumpkin, ARRAY_COUNT(pumpkin), true));
	walls.push_back(CreateTableChain(cauldron, ARRAY_COUNT(cauldron), true));
	walls.push_back(CreateTableChain(candy_bank, ARRAY_COUNT(candy_bank), true));

	// Bouncy elements: they give points
	slingshots[0] = CreateTableWall(left_sling_face, ARRAY_COUNT(left_sling_face), SLINGSHOT_RESTITUTION);
	slingshots[1] = CreateTableWall(right_sling_face, ARRAY_COUNT(right_sling_face), SLINGSHOT_RESTITUTION);
	for (int i = 0; i < SLINGSHOT_COUNT; ++i)
		slingshots[i]->listener = this;

	for (int i = 0; i < BUMPER_COUNT; ++i)
	{
		bumpers[i] = App->physics->CreateStaticCircle(TableX((float)bumper_positions[i * 2]), TableY((float)bumper_positions[i * 2 + 1]),
			TableLength(BUMPER_RADIUS), BUMPER_RESTITUTION);
		bumpers[i]->listener = this;
	}

	// Sensors: they detect the ball without blocking it
	for (int i = 0; i < TARGET_COUNT; ++i)
	{
		pumpkin_targets.sensors[i] = App->physics->CreateRectangleSensor(
			TableX((float)pumpkin_target_positions[i * 2]), TableY((float)pumpkin_target_positions[i * 2 + 1]),
			TableLength(PUMPKIN_TARGET_HALF_W), TableLength(PUMPKIN_TARGET_HALF_H));
		pumpkin_targets.sensors[i]->listener = this;

		candy_targets.sensors[i] = App->physics->CreateRectangleSensor(
			TableX((float)candy_target_positions[i * 2]), TableY((float)candy_target_positions[i * 2 + 1]),
			TableLength(CANDY_TARGET_HALF_W), TableLength(CANDY_TARGET_HALF_H));
		candy_targets.sensors[i]->listener = this;
	}

	sinkhole = App->physics->CreateRectangleSensor(TableX(SINKHOLE_X), TableY(SINKHOLE_Y),
		TableLength(SINKHOLE_HALF_SIZE), TableLength(SINKHOLE_HALF_SIZE));
	sinkhole->listener = this;

	left_flipper = new Flipper(App->physics, TableX(LEFT_FLIPPER_X), TableY(FLIPPERS_Y), TableLength(FLIPPER_LENGTH), true, flipper_tex);
	right_flipper = new Flipper(App->physics, TableX(RIGHT_FLIPPER_X), TableY(FLIPPERS_Y), TableLength(FLIPPER_LENGTH), false, flipper_tex);

	kicker = new Kicker(App->physics, TableX(KICKER_X), TableY(KICKER_Y), TableLength(KICKER_HALF_WIDTH), TableLength(KICKER_MAX_PULL), kicker_tex);

	// Sensor below the flippers: a ball touching it is lost
	drain = App->physics->CreateRectangleSensor(SCREEN_WIDTH / 2, TableY(DRAIN_Y), SCREEN_WIDTH / 2, 20);
	drain->listener = this;
}

void ModuleGame::DestroyTable()
{
	for (PhysBody* wall : walls)
		App->physics->DestroyBody(wall);
	walls.clear();

	for (int i = 0; i < SLINGSHOT_COUNT; ++i)
	{
		App->physics->DestroyBody(slingshots[i]);
		slingshots[i] = NULL;
	}

	for (int i = 0; i < BUMPER_COUNT; ++i)
	{
		App->physics->DestroyBody(bumpers[i]);
		bumpers[i] = NULL;
	}

	for (int i = 0; i < TARGET_COUNT; ++i)
	{
		App->physics->DestroyBody(pumpkin_targets.sensors[i]);
		pumpkin_targets.sensors[i] = NULL;
		App->physics->DestroyBody(candy_targets.sensors[i]);
		candy_targets.sensors[i] = NULL;
	}

	App->physics->DestroyBody(sinkhole);
	sinkhole = NULL;

	App->physics->DestroyBody(drain);
	drain = NULL;

	delete left_flipper;
	left_flipper = NULL;

	delete right_flipper;
	right_flipper = NULL;

	delete kicker;
	kicker = NULL;
}

void ModuleGame::StartRound()
{
	LOG("Round start");

	score = 0;
	balls_left = BALLS_PER_ROUND;
	state = GameState::PLAYING;
	state_timer = 0.0f;

	pumpkin_targets.Reset();
	candy_targets.Reset();
	message = NULL;
	message_timer = 0.0f;

	SpawnBall();
}

void ModuleGame::EndRound()
{
	LOG("Round over: %d points", score);

	previous_score = score;
	if (score > high_score)
		high_score = score;

	state = GameState::ROUND_OVER;
	state_timer = ROUND_OVER_TIME;

	App->audio->PlayFx(fx_round_over);
}

void ModuleGame::SpawnBall()
{
	DestroyBall();

	// Resting on the kicker plate
	int y = TableY(KICKER_Y) - TableLength(BALL_RADIUS) - 10;
	ball = new Ball(App->physics, TableX(KICKER_X), y, TableLength(BALL_RADIUS));
	ball_lost = false;

	ApplyBallMaterial();
}

void ModuleGame::ApplyBallMaterial()
{
	if (ball == NULL)
		return;

	ball->body->SetRestitution(ball_restitution);
	ball->body->SetFriction(ball_friction);
}

void ModuleGame::DestroyBall()
{
	delete ball;
	ball = NULL;
	ball_lost = false;
	sinkhole_entered = false;
	sinkhole_timer = 0.0f;
	sinkhole_cooldown = 0.0f;
}

void ModuleGame::AddScore(int points)
{
	if (state == GameState::PLAYING)
		score += points;
}

// Update: game logic and draw the scene
update_status ModuleGame::Update()
{
	float dt = GetFrameTime();

	HandleInput();
	UpdateEffects(dt);

	switch (state)
	{
	case GameState::TITLE_SCREEN:
		if (IsKeyPressed(KEY_ENTER))
			StartRound();
		break;

	case GameState::PLAYING:
		UpdatePlaying(dt);
		break;

	case GameState::ROUND_OVER:
		state_timer -= dt;
		if (state_timer <= 0.0f || IsKeyPressed(KEY_ENTER))
			StartRound();
		break;
	}

	// Draw ------------------------------------------------------------------
	DrawTextureEx(background_tex, Vector2{ table_offset_x, 0.0f }, 0.0f, table_scale, WHITE);
	DrawLights();

	kicker->Draw();
	left_flipper->Draw();
	right_flipper->Draw();

	if (ball != NULL)
		ball->Draw();

	return UPDATE_CONTINUE;
}

// PostUpdate: the HUD goes on top of everything, debug shapes included
update_status ModuleGame::PostUpdate()
{
	DrawHud();

	return UPDATE_CONTINUE;
}

void ModuleGame::HandleInput()
{
	left_flipper->SetActive(IsKeyDown(KEY_LEFT));
	right_flipper->SetActive(IsKeyDown(KEY_RIGHT));

	if (IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_RIGHT))
		App->audio->PlayFx(fx_flipper);

	if (IsKeyDown(KEY_DOWN))
	{
		kicker->Pull();
	}
	else if (IsKeyReleased(KEY_DOWN))
	{
		if (kicker->GetPull() > 0.1f)
			App->audio->PlayFx(fx_kicker);
		kicker->Release();
	}

	HandleSettingsInput();
}

// Clamp helper for the settings: keeps one decimal to avoid float drift (0.30000001...)
static float StepValue(float value, float step, float min, float max)
{
	value = roundf((value + step) * 10.0f) / 10.0f;
	return (value < min) ? min : (value > max) ? max : value;
}

void ModuleGame::HandleSettingsInput()
{
	// Gravity: F4 / F5
	if (IsKeyPressed(KEY_F4))
		App->physics->SetGravity(StepValue(App->physics->GetGravity(), -GRAVITY_STEP, GRAVITY_MIN, GRAVITY_MAX));
	if (IsKeyPressed(KEY_F5))
		App->physics->SetGravity(StepValue(App->physics->GetGravity(), GRAVITY_STEP, GRAVITY_MIN, GRAVITY_MAX));

	// Ball bounciness (restitution): F6 / F7
	if (IsKeyPressed(KEY_F6) || IsKeyPressed(KEY_F7))
	{
		float step = IsKeyPressed(KEY_F6) ? -MATERIAL_STEP : MATERIAL_STEP;
		ball_restitution = StepValue(ball_restitution, step, MATERIAL_MIN, MATERIAL_MAX);
		ApplyBallMaterial();
	}

	// Ball friction: F8 / F9
	if (IsKeyPressed(KEY_F8) || IsKeyPressed(KEY_F9))
	{
		float step = IsKeyPressed(KEY_F8) ? -MATERIAL_STEP : MATERIAL_STEP;
		ball_friction = StepValue(ball_friction, step, MATERIAL_MIN, MATERIAL_MAX);
		ApplyBallMaterial();
	}

	// Back to defaults: F11 (F10 is a Windows system key that activates the window menu)
	if (IsKeyPressed(KEY_F11))
	{
		App->physics->SetGravity(GRAVITY_Y);
		ball_restitution = BALL_RESTITUTION_DEFAULT;
		ball_friction = BALL_FRICTION_DEFAULT;
		ApplyBallMaterial();
	}

	// Target FPS: the physics keeps running in real time thanks to the fixed timestep
	if (IsKeyPressed(KEY_F2) || IsKeyPressed(KEY_F3))
	{
		int current = 0;
		for (int i = 0; i < fps_options_count; ++i)
		{
			if (fps_options[i] == App->window->GetTargetFps())
				current = i;
		}

		if (IsKeyPressed(KEY_F2) && current > 0)
			--current;
		if (IsKeyPressed(KEY_F3) && current < fps_options_count - 1)
			++current;

		App->window->SetTargetFps(fps_options[current]);
	}
}

void ModuleGame::UpdatePlaying(float dt)
{
	// Safety net: a ball that escaped the table counts as lost too
	if (ball != NULL && !ball_lost)
	{
		int x, y;
		ball->body->GetPosition(x, y);
		if (x < TableX(0.0f) || x > TableX((float)background_tex.width) || y < 0 || y > SCREEN_HEIGHT + 100)
			ball_lost = true;
	}

	if (ball_lost)
	{
		DestroyBall();
		--balls_left;

		if (balls_left > 0)
		{
			state_timer = BALL_RESPAWN_TIME;
			App->audio->PlayFx(fx_drain);
		}
		else
		{
			EndRound();
		}
	}
	else if (ball == NULL && state_timer > 0.0f)
	{
		state_timer -= dt;
		if (state_timer <= 0.0f)
			SpawnBall();
	}
	else if (ball != NULL)
	{
		UpdateSinkhole(dt);
	}
}

void ModuleGame::UpdateSinkhole(float dt)
{
	if (sinkhole_cooldown > 0.0f)
		sinkhole_cooldown -= dt;

	// Capture: done here and not in the callback because moving bodies during Step is not allowed
	if (sinkhole_entered)
	{
		sinkhole_entered = false;

		if (sinkhole_timer <= 0.0f && sinkhole_cooldown <= 0.0f)
		{
			sinkhole_timer = SINKHOLE_HOLD_TIME;
			ball->body->SetGravityScale(0.0f);
			AddScore(SINKHOLE_POINTS);
			App->audio->PlayFx(fx_hole);
			ShowMessage("SPOOKY HOLE +500");
		}
	}

	if (sinkhole_timer > 0.0f)
	{
		// Keep it still at the hole (walls could push it away)
		ball->body->SetPosition(TableX(SINKHOLE_HOLD_X), TableY(SINKHOLE_HOLD_Y));
		ball->body->SetLinearVelocity(0.0f, 0.0f);

		sinkhole_timer -= dt;
		if (sinkhole_timer <= 0.0f)
		{
			ball->body->SetGravityScale(1.0f);
			ball->body->SetLinearVelocity(SINKHOLE_EJECT_VX, SINKHOLE_EJECT_VY);
			sinkhole_cooldown = SINKHOLE_COOLDOWN;
			App->audio->PlayFx(fx_kicker);
		}
	}
}

void ModuleGame::UpdateEffects(float dt)
{
	for (int i = 0; i < BUMPER_COUNT; ++i)
		bumper_flash[i] -= dt;

	for (int i = 0; i < SLINGSHOT_COUNT; ++i)
		slingshot_flash[i] -= dt;

	if (message_timer > 0.0f)
		message_timer -= dt;
}

bool ModuleGame::HitTarget(TargetBank& bank, int index, int points)
{
	if (bank.lit[index])
	{
		AddScore(TARGET_LIT_POINTS);
		return false;
	}

	bank.lit[index] = true;
	AddScore(points);
	App->audio->PlayFx(fx_target);

	return bank.LitCount() == TARGET_COUNT;
}

void ModuleGame::ShowMessage(const char* text)
{
	message = text;
	message_timer = MESSAGE_TIME;
}

void ModuleGame::OnCollision(PhysBody* bodyA, PhysBody* bodyB)
{
	// Called during Step: the world is locked, so we only update the score and raise flags.
	// Bodies are destroyed or moved later in Update()
	if (ball == NULL || bodyB != ball->body || state != GameState::PLAYING)
		return;

	if (bodyA == drain)
	{
		ball_lost = true;
		return;
	}

	if (bodyA == sinkhole)
	{
		sinkhole_entered = true;
		return;
	}

	for (int i = 0; i < BUMPER_COUNT; ++i)
	{
		if (bodyA == bumpers[i])
		{
			AddScore(BUMPER_POINTS);
			bumper_flash[i] = FLASH_TIME;
			App->audio->PlayFx(fx_bumper);
			return;
		}
	}

	for (int i = 0; i < SLINGSHOT_COUNT; ++i)
	{
		if (bodyA == slingshots[i])
		{
			AddScore(SLINGSHOT_POINTS);
			slingshot_flash[i] = FLASH_TIME;
			App->audio->PlayFx(fx_slingshot);
			return;
		}
	}

	for (int i = 0; i < TARGET_COUNT; ++i)
	{
		if (bodyA == pumpkin_targets.sensors[i])
		{
			// Combo: carving the whole pumpkin gives an extra ball
			if (HitTarget(pumpkin_targets, i, TARGET_POINTS))
			{
				pumpkin_targets.Reset();
				AddScore(PUMPKIN_COMBO_POINTS);
				if (balls_left < MAX_BALLS)
					++balls_left;
				App->audio->PlayFx(fx_extra_ball);
				ShowMessage("PUMPKIN CARVED! EXTRA BALL");
			}
			return;
		}

		if (bodyA == candy_targets.sensors[i])
		{
			if (HitTarget(candy_targets, i, TARGET_POINTS))
			{
				candy_targets.Reset();
				AddScore(CANDY_COMBO_POINTS);
				App->audio->PlayFx(fx_bonus);
				ShowMessage("CANDY BONUS +2000");
			}
			return;
		}
	}
}

void ModuleGame::DrawLights() const
{
	const Color glow = Color{ 255, 150, 20, 255 };

	// Pumpkin progress: light the painted triangles
	for (int i = 0; i < TARGET_COUNT; ++i)
	{
		if (!pumpkin_targets.lit[i])
			continue;

		float x = (float)TableX((float)pumpkin_light_positions[i * 2]);
		float y = (float)TableY((float)pumpkin_light_positions[i * 2 + 1]);
		float s = (float)TableLength(11.0f);
		DrawCircle((int)x, (int)y, s * 1.6f, Fade(glow, 0.35f));
		DrawTriangle(Vector2{ x, y - s }, Vector2{ x - s, y + s * 0.8f }, Vector2{ x + s, y + s * 0.8f }, glow);
	}

	// Candy cards
	for (int i = 0; i < TARGET_COUNT; ++i)
	{
		if (!candy_targets.lit[i])
			continue;

		const int* r = &candy_card_rects[i * 4];
		DrawRectangle(TableX((float)r[0]), TableY((float)r[1]), TableLength((float)r[2]), TableLength((float)r[3]), Fade(YELLOW, 0.45f));
	}

	// Bumpers flash when hit
	for (int i = 0; i < BUMPER_COUNT; ++i)
	{
		if (bumper_flash[i] <= 0.0f)
			continue;

		float alpha = bumper_flash[i] / FLASH_TIME;
		Vector2 center = { (float)TableX((float)bumper_positions[i * 2]), (float)TableY((float)bumper_positions[i * 2 + 1]) };
		float radius = (float)TableLength(BUMPER_RADIUS);
		DrawRing(center, radius, radius + 6.0f, 0.0f, 360.0f, 32, Fade(WHITE, alpha));
		DrawCircleV(center, radius, Fade(SKYBLUE, alpha * 0.4f));
	}

	// Slingshot faces flash when hit
	const int* faces[SLINGSHOT_COUNT] = { left_sling_face, right_sling_face };
	for (int i = 0; i < SLINGSHOT_COUNT; ++i)
	{
		if (slingshot_flash[i] <= 0.0f)
			continue;

		float alpha = slingshot_flash[i] / FLASH_TIME;
		Vector2 a = { (float)TableX((float)faces[i][0]), (float)TableY((float)faces[i][1]) };
		Vector2 b = { (float)TableX((float)faces[i][2]), (float)TableY((float)faces[i][3]) };
		DrawLineEx(a, b, 6.0f, Fade(WHITE, alpha));
	}
}

void ModuleGame::DrawHud() const
{
	const int left_x = 40;
	const int right_x = TableX((float)background_tex.width) + 40;
	const Color text = RAYWHITE;
	const Color accent = Color{ 255, 170, 40, 255 };

	// Scores ----------------------------------------------------------------
	DrawText("SCORE", left_x, 60, 24, accent);
	DrawText(TextFormat("%d", score), left_x, 90, 40, text);

	DrawText("PREVIOUS", left_x, 160, 24, accent);
	DrawText(TextFormat("%d", previous_score), left_x, 190, 32, text);

	DrawText("HIGH SCORE", left_x, 250, 24, accent);
	DrawText(TextFormat("%d", high_score), left_x, 280, 32, text);

	DrawText("BALLS", left_x, 340, 24, accent);
	for (int i = 0; i < balls_left; ++i)
	{
		DrawCircle(left_x + 14 + i * 34, 385, 12.0f, Color{ 90, 200, 40, 255 });
	}

	// Kicker power ----------------------------------------------------------
	DrawText("POWER", left_x, 430, 24, accent);
	DrawRectangleLines(left_x, 460, 200, 20, text);
	DrawRectangle(left_x + 2, 462, (int)(196 * kicker->GetPull()), 16, accent);

	// Combo progress ----------------------------------------------------------
	DrawText("CARVE THE PUMPKIN", left_x, 520, 20, accent);
	DrawText(TextFormat("%d / %d  -> extra ball", pumpkin_targets.LitCount(), TARGET_COUNT), left_x, 545, 20, text);
	DrawText("CANDY CARDS", left_x, 585, 20, accent);
	DrawText(TextFormat("%d / %d  -> +%d", candy_targets.LitCount(), TARGET_COUNT, CANDY_COMBO_POINTS), left_x, 610, 20, text);

	// Controls and settings -------------------------------------------------
	DrawText("CONTROLS", right_x, 60, 24, accent);
	DrawText("LEFT / RIGHT  flippers", right_x, 95, 20, text);
	DrawText("DOWN  hold to pull kicker", right_x, 120, 20, text);
	DrawText("F1  debug + mouse joint", right_x, 145, 20, text);

	// Runtime physics settings ----------------------------------------------
	DrawText("SETTINGS", right_x, 200, 24, accent);
	DrawText(TextFormat("F2 / F3  target FPS: %d", App->window->GetTargetFps()), right_x, 235, 20, text);
	DrawText(TextFormat("F4 / F5  gravity: %.1f m/s2", App->physics->GetGravity()), right_x, 260, 20, text);
	DrawText(TextFormat("F6 / F7  ball bounce: %.1f", ball_restitution), right_x, 285, 20, text);
	DrawText(TextFormat("F8 / F9  ball friction: %.1f", ball_friction), right_x, 310, 20, text);
	DrawText("F11  reset settings", right_x, 335, 20, text);

	if (App->physics->IsDebug())
		DrawText("DEBUG", right_x, 380, 24, RED);

	// State messages --------------------------------------------------------
	const int center_x = SCREEN_WIDTH / 2;

	if (message != NULL && message_timer > 0.0f)
	{
		float alpha = (message_timer < 0.5f) ? message_timer / 0.5f : 1.0f;
		int width = MeasureText(message, 30);
		DrawRectangle(center_x - width / 2 - 16, 400, width + 32, 50, Fade(BLACK, 0.6f * alpha));
		DrawText(message, center_x - width / 2, 410, 30, Fade(accent, alpha));
	}

	if (state == GameState::TITLE_SCREEN)
	{
		DrawRectangle(TableX(60), 280, TableLength(540), 140, Fade(BLACK, 0.7f));
		const char* title = "HALLOWEEN PINBALL";
		DrawText(title, center_x - MeasureText(title, 36) / 2, 305, 36, accent);
		const char* start = "Press ENTER to start";
		DrawText(start, center_x - MeasureText(start, 24) / 2, 365, 24, text);
	}
	else if (state == GameState::ROUND_OVER)
	{
		DrawRectangle(TableX(60), 280, TableLength(540), 160, Fade(BLACK, 0.7f));
		const char* over = "ROUND OVER";
		DrawText(over, center_x - MeasureText(over, 36) / 2, 300, 36, accent);
		const char* result = TextFormat("Score: %d", previous_score);
		DrawText(result, center_x - MeasureText(result, 28) / 2, 350, 28, text);
		const char* next = TextFormat("Next round in %d...", (int)state_timer + 1);
		DrawText(next, center_x - MeasureText(next, 20) / 2, 400, 20, text);
	}
}
