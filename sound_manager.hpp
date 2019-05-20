#pragma once

enum sound_sources_ids
{
	sound_music,
	sound_hit,
	sound_jump,

	sound_count
};

struct sound_system
{
	// TODO(george): Free memory if sound isn't needed. Can use irrklang::...::drop() for this;
	//				 Load sounds after sound engine initialization;
	//				 Edit sound (like sound volume). Can use ISoundSources for this;
	//				 Random sound depending on a sound type (can have sound_hit1, sound_hit2 & choose randomly between them);

	irrklang::ISoundEngine *Engine;

	irrklang::ISoundSource *Sources[sound_count];

	void InitAndLoadSounds();

	void Update(v3 PlayerPos, v3 PlayerDir, v3 UpVector = V3(0.0f, 1.0f, 0.0f));
	void PlaySound2D(sound_sources_ids ID, bool32 Looped = false);
	void PlaySound3D(sound_sources_ids ID, v3 Position, bool32 Looped = false);
};

void
sound_system::InitAndLoadSounds()
{
	Engine = irrklang::createIrrKlangDevice();

	Sources[sound_hit] = Engine->addSoundSourceFromFile("data/sound/Hit_00.wav", irrklang::ESM_AUTO_DETECT, true);
	Sources[sound_jump] = Engine->addSoundSourceFromFile("data/sound/Jump_00.wav", irrklang::ESM_AUTO_DETECT, true);
	Sources[sound_music] = Engine->addSoundSourceFromFile("data/sound/Music.wav", irrklang::ESM_AUTO_DETECT, true);
}

void
sound_system::Update(v3 PlayerPos, v3 PlayerDir, v3 UpVector)
{
	irrklang::vec3df Position(PlayerPos.x, PlayerPos.y, PlayerPos.z);
	irrklang::vec3df LookDirection(PlayerDir.x, PlayerDir.y, PlayerDir.z);
	irrklang::vec3df VelPerSecond(0, 0, 0);
	irrklang::vec3df UpVec(UpVector.x, UpVector.y, UpVector.z);

	Engine->setListenerPosition(Position, LookDirection, VelPerSecond, UpVec);
}

void
sound_system::PlaySound2D(sound_sources_ids ID, bool32 Looped)
{
	Engine->play2D(Sources[ID], Looped);
}

void
sound_system::PlaySound3D(sound_sources_ids ID, v3 Position, bool32 Looped)
{
	irrklang::vec3df irrklangPos(Position.x, Position.y, Position.z);

	Engine->play3D(Sources[ID], irrklangPos, Looped);
}