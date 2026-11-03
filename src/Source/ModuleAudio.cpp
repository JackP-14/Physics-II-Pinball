#include "Globals.h"
#include "Application.h"
#include "ModuleAudio.h"

#include "raylib.h"

ModuleAudio::ModuleAudio(Application* app, bool start_enabled) : Module(app, start_enabled)
{
	fx_count = 0;
	music = Music{ 0 };
}

// Destructor
ModuleAudio::~ModuleAudio()
{}

// Called before render is available
bool ModuleAudio::Init()
{
	LOG("Loading Audio Mixer");
	bool ret = true;

    LOG("Loading raylib audio system");

    InitAudioDevice();

	return ret;
}

// Called before quitting
bool ModuleAudio::CleanUp()
{
	LOG("Freeing sound FX, closing Mixer and Audio subsystem");

    // Unload sounds
	for (unsigned int i = 0; i < fx_count; i++)
	{
		UnloadSound(fx[i]);
	}

    // Unload music
	if (IsMusicReady(music))
	{
		StopMusicStream(music);
		UnloadMusicStream(music);
	}

    CloseAudioDevice();

	return true;
}

// Play a music file
bool ModuleAudio::PlayMusic(const char* path, float fade_time)
{
	if(IsEnabled() == false)
		return false;

	bool ret = true;
	
    StopMusicStream(music);
    music = LoadMusicStream(path);
    
    PlayMusicStream(music);

	LOG("Successfully playing %s", path);

	return ret;
}

// Load WAV. Returns 0 on failure, otherwise the fx id to use with PlayFx (1-based)
unsigned int ModuleAudio::LoadFx(const char* path)
{
	if(IsEnabled() == false)
		return 0;

	unsigned int ret = 0;

	if (fx_count >= MAX_SOUNDS)
	{
		LOG("Cannot load sound %s: max sounds (%d) reached", path, MAX_SOUNDS);
		return 0;
	}

	Sound sound = LoadSound(path);

	if(sound.stream.buffer == NULL)
	{
		LOG("Cannot load sound: %s", path);
	}
	else
	{
        fx[fx_count++] = sound;
		ret = fx_count;
	}

	return ret;
}

// Play WAV
bool ModuleAudio::PlayFx(unsigned int id, int repeat)
{
	if (IsEnabled() == false)
	{
		return false;
	}

	bool ret = false;

	// Ids are 1-based (0 means "not loaded")
	if (id > 0 && id <= fx_count)
	{
		PlaySound(fx[id - 1]);
		ret = true;
	}

	return ret;
}