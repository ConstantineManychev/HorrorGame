#ifndef __AUDIO_MANAGER_H__
#define __AUDIO_MANAGER_H__

#include "cocos2d.h"
#include "CommonDefines.h" 
#include "Basics/ServiceLocator.h"
#include <vector>
#include <unordered_map>
#include <string>

_CSTART

enum class SoundType {
	MUSIC,
	SUSPENSE,
	SFX,
	UI
};

class AudioManager
{
	friend class ServiceLocator;

public:
	static AudioManager* getInstance();

	void init();

	void playMusic(const std::string& aFileName, bool aLoop = true);
	void stopMusic();
	void pauseMusic();
	void resumeMusic();

	int playSuspense(const std::string& aFileName, bool aLoop = true);
	int playSFX(const std::string& aFileName, bool aLoop = false);

	int playUI(const std::string& aFileName, bool aLoop = false);

	void stopSound(int aSoundID);
	void pauseAllGameSounds();
	void resumeAllGameSounds();

	void setVolume(SoundType aType, float aVolume);
	float getVolume(SoundType aType) const;

private:
	AudioManager();
	~AudioManager();

	int mCurrentMusicID;
	int mPauseMenuMusicID;

	float mMusicVolume;
	float mSuspenseVolume;
	float mSFXVolume;

	std::vector<int> mSuspenseIDs;
	std::vector<int> mSFXIDs;

	void cleanupInvalidIDs();
};

#define AM SL->getService<AudioManager>()

_CEND

#endif