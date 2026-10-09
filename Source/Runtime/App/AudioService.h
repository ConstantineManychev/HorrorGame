#pragma once

#include <string>

namespace hg
{
    class AudioService
    {
    public:
        void playMusic(const std::string& aFile, bool aLoop = true);
        void stopMusic();
        void playSound(const std::string& aFile);
        void stopAll();
        void pauseAll();
        void resumeAll();

        void setMusicVolume(float aVolume);
        void setSoundVolume(float aVolume);
        const std::string& currentMusic() const;

    private:
        int mMusicId = -1;
        std::string mMusicFile;
        float mMusicVolume = 1.0f;
        float mSoundVolume = 1.0f;
    };
}
