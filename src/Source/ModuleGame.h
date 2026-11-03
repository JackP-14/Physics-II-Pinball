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
#define BALL_SAVE_TIME 15.0f   // seconds after a launch during which a lost ball is given back
#define TARGET_COUNT 3         // elements of the "Trick or Treat" combo
#define BUMPER_COUNT 2
#define SLINGSHOT_COUNT 2
#define LETTER_COUNT 4         // letters spread over the table: all of them give an extra ball

// A bank of targets: hitting all of them completes the combo
struct TargetBank
{
	PhysBody* bodies[TARGET_COUNT];
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
	void OnCollisionEnd(PhysBody* bodyA, PhysBody* bodyB) override;

	void AddScore(int points);

private:
	// Table layout is authored in table image pixels: these convert it to screen pixels
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
	void UpdateWeb(float dt);
	void ReleaseFromWeb();
	void UpdateCauldron();
	void UpdateKickback();
	void ResetLetters();
	void CollectLetter(int index);
	void GiveExtraBall(const char* text);
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
	PhysBody* web;           // big spider web: the ball gets stuck in it
	PhysBody* cauldron_lane; // sensor at the end of the lane under the cauldron
	PhysBody* kickback;      // sensor in the left outlane
	PhysBody* letters[LETTER_COUNT];
	Flipper* left_flipper;
	Flipper* right_flipper;
	Kicker* kicker;
	Ball* ball;

	// Set inside the contact callbacks (the world is locked), handled in Update()
	bool ball_lost;
	bool web_entered;
	bool web_exited;
	bool cauldron_entered;
	bool kickback_entered;
	bool kickback_armed; // once per ball: shoots the ball back up from the left outlane
	bool ball_touched_flipper; // false right after a launch: a lane shot from the kicker is a skill shot

	// Ball save: a ball lost right after its launch is given back once
	bool ball_launched;  // the ball has left the kicker lane
	bool ball_save_used; // the current ball is already a saved one
	float ball_save_timer;

	// Combos and special zones
	TargetBank trick_or_treat; // both pumpkins + the cauldron lane: extra ball
	float web_timer;           // > 0 while the ball is stuck in the web
	float web_cooldown;        // the web cannot catch the ball again right after releasing it
	bool letter_collected[LETTER_COUNT];

	// Physics settings the player can change at runtime (gravity lives in ModulePhysics)
	float ball_restitution;
	float ball_friction;

	// Visual feedback
	float slingshot_flash[SLINGSHOT_COUNT];
	float cauldron_flash;
	const char* message;
	float message_timer;

	// Sound effects
	uint32 fx_bumper;
	uint32 fx_slingshot;
	uint32 fx_flipper;
	uint32 fx_kicker;
	uint32 fx_target;
	uint32 fx_cauldron;
	uint32 fx_web;
	uint32 fx_drain;
	uint32 fx_extra_ball;
	uint32 fx_round_over;
	uint32 fx_pickup;

	// Table placement on screen
	float table_scale;
	float table_offset_x;

	// Assets: every table image has the size of the table (sprites are drawn in place)
	Texture2D background_tex;
	Texture2D flipper_tex;
	Texture2D spring_tex;
	Texture2D cauldron_tex;
	Texture2D letters_tex;
};
