#pragma once

#include "Globals.h"
#include "Module.h"

#include "raylib.h"
#include <vector>

class PhysBody;
class Ball;
class Flipper;
class Kicker;

#define BALLS_PER_ROUND 3
#define MAX_BALLS 5
#define BALL_RESPAWN_TIME 1.0f // seconds between losing a ball and getting the next one
#define ROUND_OVER_TIME 4.0f   // seconds the round summary is shown before the next round starts
#define TARGET_COUNT 3         // targets in each bank (pumpkin and candy)
#define BUMPER_COUNT 3
#define SLINGSHOT_COUNT 2

// A bank of targets: hitting all of them completes the combo
struct TargetBank
{
	PhysBody* sensors[TARGET_COUNT];
	bool lit[TARGET_COUNT];

	int LitCount() const;
	void Reset();
};

enum class GameState
{
	TITLE_SCREEN, // Waiting for the first round
	PLAYING,
	ROUND_OVER  // Summary of the last round, then a new round starts automatically
};

class ModuleGame : public Module
{
public:
	ModuleGame(Application* app, bool start_enabled = true);
	~ModuleGame();

	bool Start();
	update_status Update();
	update_status PostUpdate();
	bool CleanUp();

	void OnCollision(PhysBody* bodyA, PhysBody* bodyB) override;

	void AddScore(int points);

private:
	// Table layout is authored in background image pixels: these convert it to screen pixels
	int TableX(float x) const;
	int TableY(float y) const;
	int TableLength(float length) const;
	std::vector<int> TableToScreen(const int* points, int size) const;
	PhysBody* CreateTableChain(const int* points, int size, bool loop, float restitution = 0.2f);
	PhysBody* CreateTableWall(const int* points, int size, float restitution = 0.2f);

	void CreateTable();
	void DestroyTable();

	void StartRound();
	void EndRound();
	void SpawnBall();
	void DestroyBall();

	void HandleInput();
	void HandleSettingsInput();
	void ApplyBallMaterial();
	void UpdatePlaying(float dt);
	void UpdateSinkhole(float dt);
	void UpdateEffects(float dt);

	// Returns true when the hit completed the bank
	bool HitTarget(TargetBank& bank, int index, int points);
	void ShowMessage(const char* text);

	void DrawLights() const;
	void DrawHud() const;

private:
	GameState state;
	float state_timer;

	// Scores of the current game session
	int score;
	int previous_score;
	int high_score;
	int balls_left;

	// Physics objects
	std::vector<PhysBody*> walls;
	PhysBody* bumpers[BUMPER_COUNT];
	PhysBody* slingshots[SLINGSHOT_COUNT];
	PhysBody* drain;
	PhysBody* sinkhole;
	Flipper* left_flipper;
	Flipper* right_flipper;
	Kicker* kicker;
	Ball* ball;

	// Set inside the contact callbacks (the world is locked), handled in Update()
	bool ball_lost;
	bool sinkhole_entered;

	// Combos
	TargetBank pumpkin_targets; // "Carve the Pumpkin": all three give an extra ball
	TargetBank candy_targets;   // candy cards: all three give a bonus
	float sinkhole_timer;       // > 0 while the ball is held in the sinkhole
	float sinkhole_cooldown;    // the ball cannot be captured again right after leaving

	// Physics settings the player can change at runtime (gravity lives in ModulePhysics)
	float ball_restitution;
	float ball_friction;

	// Visual feedback
	float bumper_flash[BUMPER_COUNT];
	float slingshot_flash[SLINGSHOT_COUNT];
	const char* message;
	float message_timer;

	// Sound effects
	uint32 fx_bumper;
	uint32 fx_slingshot;
	uint32 fx_flipper;
	uint32 fx_kicker;
	uint32 fx_target;
	uint32 fx_hole;
	uint32 fx_drain;
	uint32 fx_extra_ball;
	uint32 fx_bonus;
	uint32 fx_round_over;

	// Background placement
	float table_scale;
	float table_offset_x;

	// Assets
	Texture2D background_tex;
	Texture2D flipper_tex;
	Texture2D kicker_tex;
};
