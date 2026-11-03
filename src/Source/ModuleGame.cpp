#include "Globals.h"
#include "Application.h"
#include "ModuleWindow.h"
#include "ModuleRender.h"
#include "ModuleGame.h"
#include "ModuleAudio.h"
#include "ModulePhysics.h"

#include <math.h>

// Table layout, in table image pixels (fondo.png, 567x720) --------------------
#define BALL_RADIUS 11
#define FLIPPER_LENGTH 70
#define LEFT_FLIPPER_X 181
#define RIGHT_FLIPPER_X 373
#define FLIPPERS_Y 587
#define KICKER_X 482
#define KICKER_Y 554             // center of the plate at rest (its top is the top of the spring)
#define KICKER_HALF_WIDTH 13
#define KICKER_MAX_PULL 26
#define DRAIN_Y 760
#define LANE_TOP_Y 205           // a ball above this line has left the kicker lane

// Bounciness of the pumpkins and the slingshots: livelier than the walls (0.2) but below 1,
// so they never add energy to the ball
#define BUMPER_RESTITUTION 0.9f
#define SLINGSHOT_RESTITUTION 0.6f

// Spider web: while the ball is inside it falls slowly, but keeps its horizontal speed
#define WEB_X 122
#define WEB_Y 232
#define WEB_RADIUS 80
#define WEB_VERTICAL_DAMPING 6.0f // only brakes the Y velocity of the stuck ball
#define WEB_GRAVITY_SCALE 0.25f
#define WEB_HOLD_TIME 1.5f        // then the web lets the ball go
#define WEB_COOLDOWN 1.0f

// Sensor at the exit of the lane under the cauldron (just below the end of the rail):
// the cauldron spits the ball back into the playfield instead of letting it fall into the outlane
// (low enough that the ball has fully passed the end of the rail, or the push would throw it against it)
#define CAULDRON_LANE_X 45
#define CAULDRON_LANE_Y 362
#define CAULDRON_LANE_HALF_W 16
#define CAULDRON_LANE_HALF_H 6
#define CAULDRON_SPIT_VX 6.0f     // m/s, towards the center of the table
#define CAULDRON_SPIT_VY -1.0f
#define CAULDRON_FLASH_TIME 0.8f

// Kickback in the left outlane (between the outer wall and the left guide): it shoots the ball
// back up into the playfield once per ball, instead of letting it drain
#define KICKBACK_X 66
#define KICKBACK_Y 515
#define KICKBACK_HALF_W 14
#define KICKBACK_HALF_H 8
#define KICKBACK_VY -9.0f         // m/s, up: enough to get back above the left guide

// Letter collectibles (letras.png): they stay collected between balls until the word is complete
#define LETTER_RADIUS 16

// Scoring
#define BUMPER_POINTS 100
#define SLINGSHOT_POINTS 10
#define TARGET_POINTS 250         // first time an element of the combo lights up
#define CAULDRON_POINTS 500
#define SKILL_SHOT_POINTS 750      // cauldron lane straight from the kicker (does not count for the combo)
#define WEB_POINTS 300
#define LETTER_POINTS 150
#define TRICK_OR_TREAT_POINTS 1500 // plus an extra ball
#define ALL_LETTERS_POINTS 1000    // plus an extra ball

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

// Outer wall: from the right side of the drain, around the kicker lane and the top,
// down the left side (lane under the cauldron) to the left side of the drain
static const int outer_wall[] = {
	330, 722,
	453, 612,	// right rail (between playfield and kicker lane)
	453, 215,
	460, 208,
	467, 215,
	467, 612,	// kicker lane
	497, 612,
	497, 150,
	492, 110,	// top arc
	478, 75,
	455, 45,
	420, 20,
	380, 5,
	330, 1,
	260, 1,
	215, 2,
	175, 8,
	140, 20,
	110, 35,
	92, 48,
	80, 55,
	45, 60,
	22, 70,
	12, 92,
	10, 140,	// cauldron lane (its entrance must stay wider than the ball)
	10, 330,
	28, 362,
	48, 392,
	50, 410,	// left outlane
	50, 570,
	240, 722
};

// Rail that separates the cauldron lane from the playfield
static const int cauldron_rail[] = {
	37, 108,
	42, 99,
	51, 98,
	55, 108,
	55, 322,
	46, 331,
	37, 322
};

// Inlane guides. The top slope goes on past the top of the flipper pivot, so the ball drops onto
// the coffin instead of resting in a pocket next to the pivot; the end wraps around the pivot
// without touching it (the flipper must never collide with them)
static const int left_guide[] = {
	83, 415,
	89, 409,
	95, 415,
	95, 487,
	186, 562,
	174, 566,
	164, 580,
	164, 602,
	83, 537
};

static const int right_guide[] = {
	410, 424,
	416, 417,
	423, 424,
	423, 575,
	390, 602,
	390, 580,
	380, 566,
	368, 562,
	410, 520
};

// Slingshots: the vertical face kicks the ball, the two diagonals are normal walls
static const int left_sling_face[] = { 184, 402, 183, 501 };
static const int left_sling_back[] = { 184, 402, 141, 461, 183, 501 };
static const int right_sling_face[] = { 327, 402, 328, 505 };
static const int right_sling_back[] = { 327, 402, 371, 467, 328, 505 };

// Pumpkin bumpers: x, y, radius
static const int bumper_shapes[BUMPER_COUNT * 3] = {
	215, 90, 28,
	350, 193, 34
};

// Cauldron sprite center, to light it up
static const Vector2 cauldron_center = { 50.0f, 119.0f };

// Letters spread over the playfield, away from walls and bumpers
static const int letter_positions[LETTER_COUNT * 2] = {
	270, 55,	// D: top, between the pumpkins
	420, 120,	// N: under the top right arc
	110, 370,	// M: left, below the web
	410, 330	// L: right, above the right inlane
};

// Each letter inside letras.png
static const Rectangle letter_sections[LETTER_COUNT] = {
	{ 247.0f, 326.0f, 30.0f, 43.0f },	// D
	{ 277.0f, 326.0f, 28.0f, 43.0f },	// N
	{ 305.0f, 326.0f, 33.0f, 43.0f },	// M
	{ 339.0f, 326.0f, 27.0f, 43.0f }	// L
};

// Coffin flipper sprite (ataud.png): the left coffin as drawn on the table and its pivot
static const Rectangle coffin_section = { 165.0f, 570.0f, 86.0f, 61.0f };
static const Vector2 coffin_pivot = { 181.0f, 587.0f };

// Spring sprite sheet (spring.png): 4 frames, from compressed to extended
#define SPRING_FRAMES 4
static const Rectangle spring_first_frame = { 370.0f, 548.0f, 31.0f, 64.0f };
static const float spring_frame_step = 32.0f;
static const float spring_draw_y = 546.0f; // table Y where the top of the frame rect is drawn

// Render target FPS the player can cycle through
static const int fps_options[] = { 15, 30, 60, 120, 144, 240 };

#define ARRAY_COUNT(a) ((int)(sizeof(a) / sizeof(a[0])))
static const int fps_options_count = ARRAY_COUNT(fps_options);

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
	Flipper(ModulePhysics* physics, int x, int y, int length, bool _left, Texture2D _texture, float _scale)
		: PhysicEntity(physics, physics->CreateFlipper(x, y, length, _left))
		, left(_left)
		, texture(_texture)
		, scale(_scale)
	{
		// The sprite is drawn in the rest position: we rotate it by how much the body turned
		rest_angle = body->GetRotation();
	}

	void SetActive(bool active)
	{
		physics->SetFlipperActive(body, active);
	}

	void Draw() const override
	{
		int x, y;
		body->GetPosition(x, y);

		// The sprite is the left coffin: mirror it for the right one
		Rectangle source = coffin_section;
		Vector2 origin = { coffin_pivot.x - coffin_section.x, coffin_pivot.y - coffin_section.y };
		if (!left)
		{
			source.width = -source.width;
			origin.x = coffin_section.width - origin.x;
		}

		Rectangle dest = { (float)x, (float)y, coffin_section.width * scale, coffin_section.height * scale };
		origin.x *= scale;
		origin.y *= scale;
		DrawTexturePro(texture, source, dest, origin, (body->GetRotation() - rest_angle) * RAD2DEG, WHITE);
	}

private:
	bool left;
	float rest_angle;
	Texture2D texture;
	float scale;
};

class Kicker : public PhysicEntity
{
public:
	Kicker(ModulePhysics* physics, int x, int y, int half_width, int max_pull, Texture2D _texture, float _scale, float _draw_x, float _draw_y)
		: PhysicEntity(physics, physics->CreateKicker(x, y, half_width, max_pull))
		, texture(_texture)
		, scale(_scale)
		, draw_x(_draw_x)
		, draw_y(_draw_y)
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
		// The more it is pulled, the more compressed the spring frame
		int frame = (SPRING_FRAMES - 1) - (int)(GetPull() * (SPRING_FRAMES - 1) + 0.5f);

		Rectangle source = spring_first_frame;
		source.x += frame * spring_frame_step;

		Rectangle dest = { draw_x - source.width * scale / 2.0f, draw_y, source.width * scale, source.height * scale };
		DrawTexturePro(texture, source, dest, Vector2{ 0.0f, 0.0f }, 0.0f, WHITE);
	}

private:
	Texture2D texture;
	float scale;
	float draw_x; // screen position of the spring: center X and top Y
	float draw_y;
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
		bumpers[i] = NULL;
	for (int i = 0; i < SLINGSHOT_COUNT; ++i)
	{
		slingshots[i] = NULL;
		slingshot_flash[i] = 0.0f;
	}
	for (int i = 0; i < TARGET_COUNT; ++i)
		trick_or_treat.bodies[i] = NULL;
	trick_or_treat.Reset();

	for (int i = 0; i < LETTER_COUNT; ++i)
	{
		letters[i] = NULL;
		letter_collected[i] = false;
	}

	cauldron_entered = false;
	cauldron_flash = 0.0f;
	kickback = NULL;
	kickback_entered = false;
	kickback_armed = false;
	ball_touched_flipper = false;
	ball_launched = false;
	ball_save_used = false;
	ball_save_timer = 0.0f;

	drain = NULL;
	web = NULL;
	cauldron_lane = NULL;
	left_flipper = NULL;
	right_flipper = NULL;
	kicker = NULL;
	ball = NULL;
	ball_lost = false;
	web_entered = false;
	web_exited = false;
	web_timer = 0.0f;
	web_cooldown = 0.0f;

	message = NULL;
	message_timer = 0.0f;

	ball_restitution = BALL_RESTITUTION_DEFAULT;
	ball_friction = BALL_FRICTION_DEFAULT;

	fx_bumper = fx_slingshot = fx_flipper = fx_kicker = fx_target = 0;
	fx_cauldron = fx_web = fx_drain = fx_extra_ball = fx_round_over = fx_pickup = 0;

	table_scale = 1.0f;
	table_offset_x = 0.0f;

	background_tex = Texture2D{ 0 };
	flipper_tex = Texture2D{ 0 };
	spring_tex = Texture2D{ 0 };
	cauldron_tex = Texture2D{ 0 };
	letters_tex = Texture2D{ 0 };
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

	background_tex = LoadTexture("Assets/fondo.png");
	flipper_tex = LoadTexture("Assets/ataud.png");
	spring_tex = LoadTexture("Assets/spring.png");
	cauldron_tex = LoadTexture("Assets/caldero.png");
	letters_tex = LoadTexture("Assets/letras.png");

	// The audio module owns the sounds and unloads them
	fx_bumper = App->audio->LoadFx("Assets/Audio/Fx/bumper.wav");
	fx_slingshot = App->audio->LoadFx("Assets/Audio/Fx/slingshot.wav");
	fx_flipper = App->audio->LoadFx("Assets/Audio/Fx/flipper.wav");
	fx_kicker = App->audio->LoadFx("Assets/Audio/Fx/kicker.wav");
	fx_target = App->audio->LoadFx("Assets/Audio/Fx/target.wav");
	fx_cauldron = App->audio->LoadFx("Assets/Audio/Fx/hole.wav");
	fx_web = App->audio->LoadFx("Assets/Audio/Fx/web.wav");
	fx_drain = App->audio->LoadFx("Assets/Audio/Fx/drain.wav");
	fx_extra_ball = App->audio->LoadFx("Assets/Audio/Fx/extra_ball.wav");
	fx_round_over = App->audio->LoadFx("Assets/Audio/Fx/round_over.wav");
	fx_pickup = App->audio->LoadFx("Assets/Audio/Fx/pickup.wav");

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
	UnloadTexture(spring_tex);
	UnloadTexture(cauldron_tex);
	UnloadTexture(letters_tex);

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
	walls.push_back(CreateTableChain(cauldron_rail, ARRAY_COUNT(cauldron_rail), true));
	walls.push_back(CreateTableChain(left_guide, ARRAY_COUNT(left_guide), true));
	walls.push_back(CreateTableChain(right_guide, ARRAY_COUNT(right_guide), true));
	walls.push_back(CreateTableWall(left_sling_back, ARRAY_COUNT(left_sling_back)));
	walls.push_back(CreateTableWall(right_sling_back, ARRAY_COUNT(right_sling_back)));

	// Bouncy elements: they give points
	slingshots[0] = CreateTableWall(left_sling_face, ARRAY_COUNT(left_sling_face), SLINGSHOT_RESTITUTION);
	slingshots[1] = CreateTableWall(right_sling_face, ARRAY_COUNT(right_sling_face), SLINGSHOT_RESTITUTION);
	for (int i = 0; i < SLINGSHOT_COUNT; ++i)
		slingshots[i]->listener = this;

	for (int i = 0; i < BUMPER_COUNT; ++i)
	{
		const int* b = &bumper_shapes[i * 3];
		bumpers[i] = App->physics->CreateStaticCircle(TableX((float)b[0]), TableY((float)b[1]), TableLength((float)b[2]), BUMPER_RESTITUTION);
		bumpers[i]->listener = this;
	}

	// Sensors: they detect the ball without blocking it
	web = App->physics->CreateCircleSensor(TableX(WEB_X), TableY(WEB_Y), TableLength(WEB_RADIUS));
	web->listener = this;

	cauldron_lane = App->physics->CreateRectangleSensor(TableX(CAULDRON_LANE_X), TableY(CAULDRON_LANE_Y),
		TableLength(CAULDRON_LANE_HALF_W), TableLength(CAULDRON_LANE_HALF_H));
	cauldron_lane->listener = this;

	kickback = App->physics->CreateRectangleSensor(TableX(KICKBACK_X), TableY(KICKBACK_Y),
		TableLength(KICKBACK_HALF_W), TableLength(KICKBACK_HALF_H));
	kickback->listener = this;

	// "Trick or Treat" combo: both pumpkins and the cauldron lane
	trick_or_treat.bodies[0] = bumpers[0];
	trick_or_treat.bodies[1] = bumpers[1];
	trick_or_treat.bodies[2] = cauldron_lane;

	for (int i = 0; i < LETTER_COUNT; ++i)
	{
		letters[i] = App->physics->CreateCircleSensor(TableX((float)letter_positions[i * 2]), TableY((float)letter_positions[i * 2 + 1]), TableLength(LETTER_RADIUS));
		letters[i]->listener = this;
	}

	left_flipper = new Flipper(App->physics, TableX(LEFT_FLIPPER_X), TableY(FLIPPERS_Y), TableLength(FLIPPER_LENGTH), true, flipper_tex, table_scale);
	right_flipper = new Flipper(App->physics, TableX(RIGHT_FLIPPER_X), TableY(FLIPPERS_Y), TableLength(FLIPPER_LENGTH), false, flipper_tex, table_scale);

	// Listen to the flippers to know when a launched ball comes into play
	left_flipper->body->listener = this;
	right_flipper->body->listener = this;

	kicker = new Kicker(App->physics, TableX(KICKER_X), TableY(KICKER_Y), TableLength(KICKER_HALF_WIDTH), TableLength(KICKER_MAX_PULL),
		spring_tex, table_scale, (float)TableX(KICKER_X), (float)TableY(spring_draw_y));

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
		trick_or_treat.bodies[i] = NULL;

	App->physics->DestroyBody(web);
	web = NULL;

	App->physics->DestroyBody(cauldron_lane);
	cauldron_lane = NULL;

	App->physics->DestroyBody(kickback);
	kickback = NULL;

	for (int i = 0; i < LETTER_COUNT; ++i)
	{
		App->physics->DestroyBody(letters[i]);
		letters[i] = NULL;
	}

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

	trick_or_treat.Reset();
	ResetLetters();
	message = NULL;
	message_timer = 0.0f;
	ball_save_used = false;

	SpawnBall();
}

void ModuleGame::ResetLetters()
{
	for (int i = 0; i < LETTER_COUNT; ++i)
		letter_collected[i] = false;
}

void ModuleGame::CollectLetter(int index)
{
	if (letter_collected[index])
		return;

	letter_collected[index] = true;
	AddScore(LETTER_POINTS);
	App->audio->PlayFx(fx_pickup);

	for (int i = 0; i < LETTER_COUNT; ++i)
	{
		if (!letter_collected[i])
			return;
	}

	// All of them: they come back for another round of collecting
	ResetLetters();
	AddScore(ALL_LETTERS_POINTS);
	GiveExtraBall("DNML COMPLETE! EXTRA BALL");
}

void ModuleGame::GiveExtraBall(const char* text)
{
	if (balls_left < MAX_BALLS)
		++balls_left;
	App->audio->PlayFx(fx_extra_ball);
	ShowMessage(text);
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
	int y = TableY(KICKER_Y) - 8 - TableLength(BALL_RADIUS) - 2;
	ball = new Ball(App->physics, TableX(KICKER_X), y, TableLength(BALL_RADIUS));
	ball_lost = false;
	ball_touched_flipper = false;
	ball_launched = false;
	ball_save_timer = 0.0f;
	kickback_armed = true;

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
	// Clear the pointer first: destroying the body fires EndContact callbacks
	Ball* old_ball = ball;
	ball = NULL;
	delete old_ball;

	ball_lost = false;
	web_entered = false;
	web_exited = false;
	cauldron_entered = false;
	kickback_entered = false;
	web_timer = 0.0f;
	web_cooldown = 0.0f;
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
	Vector2 table_position = { table_offset_x, 0.0f };
	DrawTextureEx(background_tex, table_position, 0.0f, table_scale, WHITE);
	DrawLights();

	kicker->Draw();
	left_flipper->Draw();
	right_flipper->Draw();

	if (ball != NULL)
		ball->Draw();

	// The cauldron goes over the ball: the ball rolls under it
	DrawTextureEx(cauldron_tex, table_position, 0.0f, table_scale, WHITE);

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

	// Ball save starts when the ball leaves the kicker lane (only once per ball)
	if (ball != NULL && !ball_launched)
	{
		int x, y;
		ball->body->GetPosition(x, y);
		if (y < TableY(LANE_TOP_Y))
		{
			ball_launched = true;
			if (!ball_save_used)
				ball_save_timer = BALL_SAVE_TIME;
		}
	}

	if (ball_save_timer > 0.0f)
		ball_save_timer -= dt;

	if (ball_lost && ball_save_timer > 0.0f)
	{
		// Given back on the kicker without losing a ball. The saved ball cannot be saved again
		SpawnBall();
		ball_save_used = true;
		App->audio->PlayFx(fx_target);
		ShowMessage("BALL SAVED!");
	}
	else if (ball_lost)
	{
		DestroyBall();
		--balls_left;
		ball_save_used = false;

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
		UpdateWeb(dt);
		UpdateCauldron();
		UpdateKickback();
	}
}

void ModuleGame::UpdateKickback()
{
	if (!kickback_entered)
		return;
	kickback_entered = false;

	if (!kickback_armed)
		return;

	// Straight up the outlane and back into the playfield. Used until the next ball
	kickback_armed = false;
	ball->body->SetLinearVelocity(0.0f, KICKBACK_VY);
	App->audio->PlayFx(fx_kicker);
	ShowMessage("KICKBACK!");
}

void ModuleGame::UpdateCauldron()
{
	if (!cauldron_entered)
		return;
	cauldron_entered = false;

	// Only a ball coming down out of the lane counts (the kickback shoots balls up across the sensor)
	float vx, vy;
	ball->body->GetLinearVelocity(vx, vy);
	if (vy <= 0.0f)
		return;

	// The cauldron spits the ball towards the center, so the lane is not a straight way to the outlane
	ball->body->SetLinearVelocity(CAULDRON_SPIT_VX, CAULDRON_SPIT_VY);
	cauldron_flash = CAULDRON_FLASH_TIME;
	App->audio->PlayFx(fx_cauldron);

	// Straight from the kicker it is a skill shot: points, but it does not count for the combo
	if (!ball_touched_flipper)
	{
		AddScore(SKILL_SHOT_POINTS);
		ShowMessage("SKILL SHOT! +750");
		return;
	}

	AddScore(CAULDRON_POINTS);
	ShowMessage("THROUGH THE CAULDRON +500");

	if (HitTarget(trick_or_treat, 2, TARGET_POINTS))
	{
		trick_or_treat.Reset();
		AddScore(TRICK_OR_TREAT_POINTS);
		GiveExtraBall("TRICK OR TREAT! EXTRA BALL");
	}
}

void ModuleGame::UpdateWeb(float dt)
{
	if (web_cooldown > 0.0f)
		web_cooldown -= dt;

	// Done here and not in the callback: changing bodies during Step is not allowed
	if (web_entered)
	{
		web_entered = false;

		if (web_timer <= 0.0f && web_cooldown <= 0.0f)
		{
			// Sticky: vertical damping and low gravity make it fall slowly; its horizontal speed is kept
			web_timer = WEB_HOLD_TIME;
			ball->body->SetVerticalDamping(WEB_VERTICAL_DAMPING);
			ball->body->SetGravityScale(WEB_GRAVITY_SCALE);
			AddScore(WEB_POINTS);
			App->audio->PlayFx(fx_web);
			ShowMessage("STUCK IN THE WEB! +300");
		}
	}

	if (web_timer > 0.0f)
	{
		web_timer -= dt;

		// The web lets it go after a while, or when something knocks the ball out of it
		if (web_timer <= 0.0f || web_exited)
			ReleaseFromWeb();
	}

	web_exited = false;
}

void ModuleGame::ReleaseFromWeb()
{
	web_timer = 0.0f;
	web_cooldown = WEB_COOLDOWN;
	ball->body->SetVerticalDamping(0.0f);
	ball->body->SetGravityScale(1.0f);
}

void ModuleGame::UpdateEffects(float dt)
{
	for (int i = 0; i < SLINGSHOT_COUNT; ++i)
		slingshot_flash[i] -= dt;

	if (cauldron_flash > 0.0f)
		cauldron_flash -= dt;

	if (message_timer > 0.0f)
		message_timer -= dt;
}

bool ModuleGame::HitTarget(TargetBank& bank, int index, int points)
{
	if (bank.lit[index])
		return false;

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
	// Bodies are destroyed or changed later in Update()
	if (ball == NULL || bodyB != ball->body || state != GameState::PLAYING)
		return;

	if (bodyA == drain)
	{
		ball_lost = true;
		return;
	}

	if (bodyA == web)
	{
		web_entered = true;
		return;
	}

	if (bodyA == cauldron_lane)
	{
		cauldron_entered = true;
		return;
	}

	if (bodyA == kickback)
	{
		kickback_entered = true;
		return;
	}

	if (bodyA == left_flipper->body || bodyA == right_flipper->body)
	{
		ball_touched_flipper = true;
		return;
	}

	for (int i = 0; i < LETTER_COUNT; ++i)
	{
		if (bodyA == letters[i])
		{
			CollectLetter(i);
			return;
		}
	}

	int combo_index = -1;

	for (int i = 0; i < BUMPER_COUNT; ++i)
	{
		if (bodyA == bumpers[i])
		{
			AddScore(BUMPER_POINTS);
			App->audio->PlayFx(fx_bumper);
			combo_index = i;
		}
	}

	for (int i = 0; i < SLINGSHOT_COUNT; ++i)
	{
		if (bodyA == slingshots[i])
		{
			AddScore(SLINGSHOT_POINTS);
			slingshot_flash[i] = FLASH_TIME;
			App->audio->PlayFx(fx_slingshot);
		}
	}

	// Combo: both pumpkins and the cauldron lane give an extra ball
	if (combo_index >= 0 && HitTarget(trick_or_treat, combo_index, TARGET_POINTS))
	{
		trick_or_treat.Reset();
		AddScore(TRICK_OR_TREAT_POINTS);
		GiveExtraBall("TRICK OR TREAT! EXTRA BALL");
	}
}

void ModuleGame::OnCollisionEnd(PhysBody* bodyA, PhysBody* bodyB)
{
	if (ball == NULL || bodyB != ball->body)
		return;

	if (bodyA == web)
		web_exited = true;
}

void ModuleGame::DrawLights() const
{

	// Kickback ready: green arrow pointing up the left outlane
	if (kickback_armed && ball != NULL)
	{
		float x = (float)TableX(KICKBACK_X);
		float y = (float)TableY(KICKBACK_Y);
		float s = (float)TableLength(9.0f);
		float pulse = 0.6f + 0.4f * sinf((float)GetTime() * 6.0f);
		DrawTriangle(Vector2{ x, y - s }, Vector2{ x - s, y + s * 0.6f }, Vector2{ x + s, y + s * 0.6f }, Fade(Color{ 80, 255, 60, 255 }, pulse));
	}

	// The cauldron only glows for a moment, when it spits the ball
	if (cauldron_flash > 0.0f)
	{
		float alpha = cauldron_flash / CAULDRON_FLASH_TIME;
		Vector2 center = { (float)TableX(cauldron_center.x), (float)TableY(cauldron_center.y) };
		DrawCircleV(center, (float)TableLength(42.0f), Fade(Color{ 80, 255, 60, 255 }, 0.45f * alpha));
	}

	// Letters not collected yet, floating over the table
	for (int i = 0; i < LETTER_COUNT; ++i)
	{
		if (letter_collected[i])
			continue;

		float bob = 3.0f * sinf((float)GetTime() * 3.0f + i);
		Rectangle source = letter_sections[i];
		Rectangle dest = { (float)TableX((float)letter_positions[i * 2]), (float)TableY((float)letter_positions[i * 2 + 1]) + bob,
			source.width * table_scale, source.height * table_scale };
		Vector2 origin = { dest.width / 2.0f, dest.height / 2.0f };
		DrawTexturePro(letters_tex, source, dest, origin, 0.0f, WHITE);
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
	const Color done = Color{ 120, 255, 90, 255 };

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

	if (ball_save_timer > 0.0f)
		DrawText(TextFormat("BALL SAVE %d", (int)ball_save_timer + 1), left_x + 100, 344, 20, done);

	// Kicker power ----------------------------------------------------------
	DrawText("POWER", left_x, 430, 24, accent);
	DrawRectangleLines(left_x, 460, 200, 20, text);
	DrawRectangle(left_x + 2, 462, (int)(196 * kicker->GetPull()), 16, accent);

	// Combo progress ----------------------------------------------------------
	DrawText("TRICK OR TREAT (extra ball)", left_x, 520, 20, accent);
	const char* combo_names[TARGET_COUNT] = { "Top pumpkin", "Big pumpkin", "Cauldron lane" };
	for (int i = 0; i < TARGET_COUNT; ++i)
	{
		bool lit = trick_or_treat.lit[i];
		DrawText(TextFormat("[%c] %s", lit ? 'X' : ' ', combo_names[i]), left_x, 550 + i * 25, 20, lit ? done : text);
	}

	// Collected letters are drawn in color, the missing ones dimmed
	DrawText("LETTERS (extra ball)", left_x, 640, 20, accent);
	float letter_x = (float)left_x;
	for (int i = 0; i < LETTER_COUNT; ++i)
	{
		Rectangle source = letter_sections[i];
		Rectangle dest = { letter_x, 665.0f, source.width, source.height };
		DrawTexturePro(letters_tex, source, dest, Vector2{ 0.0f, 0.0f }, 0.0f, letter_collected[i] ? WHITE : Fade(GRAY, 0.35f));
		letter_x += source.width + 4.0f;
	}

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
	const int center_x = TableX(background_tex.width / 2.0f);

	if (message != NULL && message_timer > 0.0f)
	{
		float alpha = (message_timer < 0.5f) ? message_timer / 0.5f : 1.0f;
		int width = MeasureText(message, 26);
		DrawRectangle(center_x - width / 2 - 14, 340, width + 28, 44, Fade(BLACK, 0.6f * alpha));
		DrawText(message, center_x - width / 2, 349, 26, Fade(accent, alpha));
	}

	if (state == GameState::TITLE_SCREEN)
	{
		DrawRectangle(TableX(40), 280, TableLength(420), 140, Fade(BLACK, 0.7f));
		const char* title = "HALLOWEEN PINBALL";
		DrawText(title, center_x - MeasureText(title, 34) / 2, 305, 34, accent);
		const char* start = "Press ENTER to start";
		DrawText(start, center_x - MeasureText(start, 24) / 2, 365, 24, text);
	}
	else if (state == GameState::ROUND_OVER)
	{
		DrawRectangle(TableX(40), 280, TableLength(420), 160, Fade(BLACK, 0.7f));
		const char* over = "ROUND OVER";
		DrawText(over, center_x - MeasureText(over, 36) / 2, 300, 36, accent);
		const char* result = TextFormat("Score: %d", previous_score);
		DrawText(result, center_x - MeasureText(result, 28) / 2, 350, 28, text);
		const char* next = TextFormat("Next round in %d...", (int)state_timer + 1);
		DrawText(next, center_x - MeasureText(next, 20) / 2, 400, 20, text);
	}
}
