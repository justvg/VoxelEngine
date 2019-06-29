#pragma once

enum sound_source_id
{
	Sound_Music,
	Sound_Hit1,
	Sound_Hit2,
	Sound_Hit3,
	Sound_Jump,
	Sound_Spell,

	Sound_Count
};

// TODO(georgy): Add sound_source_id here to allow one entity have several sounds that can be stopped?
//			     (because we want find the exact one we need in the hash table);
struct hash_sound
{
	bool32 StorageIndex;
	irrklang::ISound *Sound;
};

struct sound_system
{
	// TODO(georgy): Load sounds after sound engine initialization; (I can just use addSoundSourceFromFile with preload param = false);
	irrklang::ISoundEngine *Engine;

	irrklang::ISoundSource *Sources[Sound_Count];

	// NOTE(georgy): Used to stop a sound in the middle of playing. Must be a power of 2!
	hash_sound HashTable[256];

	void InitAndLoadSounds();

	void Update(v3 PlayerDir, v3 UpVector = V3(0.0f, 1.0f, 0.0f));
	void PlaySound2D(sound_source_id ID, bool32 Looped = false);
	void PlaySound2DAndAddToHashTable(uint32 StorageIndex, sound_source_id ID, bool32 Looped = false);
	void PlaySound3D(sound_source_id ID, v3 Position, bool32 Looped = false);
	void AddSoundToHashTable(irrklang::ISound *Sound, uint32 StorageIndex);
	irrklang::ISound *GetSoundFromHashTableAndFreeSlot(uint32 StorageIndex);
	void StopSoundAndDeleteFromHashTable(uint32 StorageIndex);
	void PlayRandomSound2D(sound_source_id Start, sound_source_id End, bool32 Looped = false);
};

void
sound_system::AddSoundToHashTable(irrklang::ISound *Sound, uint32 StorageIndex)
{
	uint32 HashIndex = StorageIndex & (ArrayCount(HashTable) - 1);

	uint32 I = 0;
	for (; I < ArrayCount(HashTable); I++)
	{
		uint32 Index = (HashIndex + I) & (ArrayCount(HashTable) - 1);
		if (!HashTable[Index].StorageIndex)
		{
			HashTable[Index].StorageIndex = StorageIndex;
			HashTable[Index].Sound = Sound;
			break;
		}
	}
	Assert(I < ArrayCount(HashTable));
}

irrklang::ISound *
sound_system::GetSoundFromHashTableAndFreeSlot(uint32 StorageIndex)
{
	irrklang::ISound *Result = false;

	uint32 HashIndex = StorageIndex & (ArrayCount(HashTable) - 1);
	uint32 I = 0;
	for (; I < ArrayCount(HashTable); I++)
	{
		uint32 Index = (HashIndex + I) & (ArrayCount(HashTable) - 1);
		if (StorageIndex == HashTable[Index].StorageIndex)
		{
			HashTable[Index].StorageIndex = 0;
			Result = HashTable[Index].Sound;
			break;
		}
	}
	Assert(I < ArrayCount(HashTable));

	return(Result);
}

void
sound_system::InitAndLoadSounds()
{
	Engine = irrklang::createIrrKlangDevice();

	Engine->setSoundVolume(0.3f);

	Sources[Sound_Music] = Engine->addSoundSourceFromFile("data/sound/Music.mp3", irrklang::ESM_AUTO_DETECT, true);
	Sources[Sound_Hit1] = Engine->addSoundSourceFromFile("data/sound/Hit_00.wav", irrklang::ESM_AUTO_DETECT, true);
	Sources[Sound_Hit2] = Engine->addSoundSourceFromFile("data/sound/Hit_01.wav", irrklang::ESM_AUTO_DETECT, true);
	Sources[Sound_Hit3] = Engine->addSoundSourceFromFile("data/sound/Hit_02.wav", irrklang::ESM_AUTO_DETECT, true);
	Sources[Sound_Jump] = Engine->addSoundSourceFromFile("data/sound/Jump_00.wav", irrklang::ESM_AUTO_DETECT, true);
	Sources[Sound_Spell] = Engine->addSoundSourceFromFile("data/sound/Spell.wav", irrklang::ESM_AUTO_DETECT, true);
}

void
sound_system::Update(v3 PlayerDir, v3 UpVector)
{
	irrklang::vec3df Position(0.0f, 0.0f, 0.0f);
	irrklang::vec3df LookDirection(PlayerDir.x, PlayerDir.y, PlayerDir.z);
	irrklang::vec3df VelPerSecond(0, 0, 0);
	irrklang::vec3df UpVec(UpVector.x, UpVector.y, UpVector.z);

	Engine->setListenerPosition(Position, LookDirection, VelPerSecond, UpVec);
}

void
sound_system::PlaySound2D(sound_source_id ID, bool32 Looped)
{
	Engine->play2D(Sources[ID], Looped);
}

void 
sound_system::PlayRandomSound2D(sound_source_id Start, sound_source_id End, bool32 Looped)
{
	uint32 Index = Start + (rand() % (End - Start + 1));
	PlaySound2D((sound_source_id) Index, Looped);
}

void
sound_system::PlaySound3D(sound_source_id ID, v3 Position, bool32 Looped)
{
	irrklang::vec3df irrklangPos(Position.x, Position.y, Position.z);
	Engine->play3D(Sources[ID], irrklangPos, Looped);;
}

void 
sound_system::PlaySound2DAndAddToHashTable(uint32 StorageIndex, sound_source_id ID, bool32 Looped)
{
	irrklang::ISound *Sound = Engine->play2D(Sources[ID], Looped, true, true, true);
	Sound->setIsPaused(false);
	AddSoundToHashTable(Sound, StorageIndex);
}

void 
sound_system::StopSoundAndDeleteFromHashTable(uint32 StorageIndex)
{
	irrklang::ISound *Sound = GetSoundFromHashTableAndFreeSlot(StorageIndex);
	Sound->stop();
	Sound->drop();
}