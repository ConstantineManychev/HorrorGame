#include "AudioManager.h"

#include "audio/include/AudioEngine.h"
// #include "AudioEngine.h" 

USING_NS_CC;

using AudioEngine = cocos2d::AudioEngine;

_CSTART

AudioManager* AudioManager::getInstance()
{
	static AudioManager instance;
	return &instance;
}

AudioManager::AudioManager()
	: mCurrentMusicID(-1)
	, mPauseMenuMusicID(-1)
	, mMusicVolume(1.0f)
	, mSuspenseVolume(1.0f)
	, mSFXVolume(1.0f)
{
}

AudioManager::~AudioManager()
{
	AudioEngine::end();
}

void AudioManager::init()
{
}

void AudioManager::playMusic(const std::string& aFileName, bool aLoop)
{
	if (mCurrentMusicID != -1)
	{
		AudioEngine::stop(mCurrentMusicID);
	}

	mCurrentMusicID = AudioEngine::play2d(aFileName, aLoop, mMusicVolume);
}

void AudioManager::stopMusic()
{
	if (mCurrentMusicID != -1)
	{
		AudioEngine::stop(mCurrentMusicID);
		mCurrentMusicID = -1;
	}
}

void AudioManager::pauseMusic()
{
	if (mCurrentMusicID != -1)
	{
		AudioEngine::pause(mCurrentMusicID);
	}
}

void AudioManager::resumeMusic()
{
	if (mCurrentMusicID != -1)
	{
		AudioEngine::resume(mCurrentMusicID);
	}
}

int AudioManager::playSuspense(const std::string& aFileName, bool aLoop)
{
	int id = AudioEngine::play2d(aFileName, aLoop, mSuspenseVolume);
	if (id != -1)
	{
		mSuspenseIDs.push_back(id);
	}
	return id;
}

int AudioManager::playSFX(const std::string& aFileName, bool aLoop)
{
	int id = AudioEngine::play2d(aFileName, aLoop, mSFXVolume);
	if (id != -1)
	{
		mSFXIDs.push_back(id);
	}
	return id;
}

int AudioManager::playUI(const std::string& aFileName, bool aLoop)
{
	return AudioEngine::play2d(aFileName, aLoop, mSFXVolume);
}

void AudioManager::stopSound(int aSoundID)
{
	AudioEngine::stop(aSoundID);
}

void AudioManager::pauseAllGameSounds()
{
	cleanupInvalidIDs();

	if (mCurrentMusicID != -1) AudioEngine::pause(mCurrentMusicID);

	for (int id : mSuspenseIDs) AudioEngine::pause(id);
	for (int id : mSFXIDs) AudioEngine::pause(id);
}

void AudioManager::resumeAllGameSounds()
{
	cleanupInvalidIDs();

	if (mCurrentMusicID != -1) AudioEngine::resume(mCurrentMusicID);

	for (int id : mSuspenseIDs) AudioEngine::resume(id);
	for (int id : mSFXIDs) AudioEngine::resume(id);
}

void AudioManager::setVolume(SoundType aType, float aVolume)
{
	cleanupInvalidIDs();
	float clampedVol = std::max(0.0f, std::min(aVolume, 1.0f));

	switch (aType)
	{
	case SoundType::MUSIC:
		mMusicVolume = clampedVol;
		if (mCurrentMusicID != -1) AudioEngine::setVolume(mCurrentMusicID, mMusicVolume);
		break;
	case SoundType::SUSPENSE:
		mSuspenseVolume = clampedVol;
		for (int id : mSuspenseIDs) AudioEngine::setVolume(id, mSuspenseVolume);
		break;
	case SoundType::SFX:
	case SoundType::UI:
		mSFXVolume = clampedVol;
		for (int id : mSFXIDs) AudioEngine::setVolume(id, mSFXVolume);
		break;
	}
}

float AudioManager::getVolume(SoundType aType) const
{
	switch (aType)
	{
	case SoundType::MUSIC: return mMusicVolume;
	case SoundType::SUSPENSE: return mSuspenseVolume;
	case SoundType::SFX: return mSFXVolume;
	default: return 1.0f;
	}
}

void AudioManager::cleanupInvalidIDs()
{

}

_CEND