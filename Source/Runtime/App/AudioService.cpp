#include "Runtime/App/AudioService.h"

#include "Runtime/App/Log.h"

#include "audio/AudioEngine.h"
#include "axmol.h"

namespace hg
{
    namespace
    {
        bool audioFileExists(const std::string& aFile)
        {
            if (ax::FileUtils::getInstance()->isFileExist(aFile))
            {
                return true;
            }
            Log::warning("Audio file not found: " + aFile);
            return false;
        }
    }

    void AudioService::playMusic(const std::string& aFile, bool aLoop)
    {
        if (aFile == mMusicFile && mMusicId != ax::AudioEngine::INVALID_AUDIO_ID)
        {
            return;
        }
        stopMusic();
        if (aFile.empty() || !audioFileExists(aFile))
        {
            return;
        }
        mMusicId = ax::AudioEngine::play2d(aFile, aLoop, mMusicVolume);
        mMusicFile = aFile;
    }

    void AudioService::stopMusic()
    {
        if (mMusicId != ax::AudioEngine::INVALID_AUDIO_ID)
        {
            ax::AudioEngine::stop(mMusicId);
        }
        mMusicId = ax::AudioEngine::INVALID_AUDIO_ID;
        mMusicFile.clear();
    }

    void AudioService::playSound(const std::string& aFile)
    {
        if (!aFile.empty() && audioFileExists(aFile))
        {
            ax::AudioEngine::play2d(aFile, false, mSoundVolume);
        }
    }

    void AudioService::stopAll()
    {
        ax::AudioEngine::stopAll();
        mMusicId = ax::AudioEngine::INVALID_AUDIO_ID;
        mMusicFile.clear();
    }

    void AudioService::pauseAll()
    {
        ax::AudioEngine::pauseAll();
    }

    void AudioService::resumeAll()
    {
        ax::AudioEngine::resumeAll();
    }

    void AudioService::setMusicVolume(float aVolume)
    {
        mMusicVolume = aVolume;
        if (mMusicId != ax::AudioEngine::INVALID_AUDIO_ID)
        {
            ax::AudioEngine::setVolume(mMusicId, aVolume);
        }
    }

    void AudioService::setSoundVolume(float aVolume)
    {
        mSoundVolume = aVolume;
    }

    const std::string& AudioService::currentMusic() const
    {
        return mMusicFile;
    }
}
