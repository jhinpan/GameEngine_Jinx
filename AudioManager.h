#ifndef MAIN_CPP_AUDIOMANAGER_H
#define MAIN_CPP_AUDIOMANAGER_H

// This is the class to manage audio loading / playback
#include "AudioHelper.h"
#include <string>
#include <unordered_map>

class AudioManager {
public:
    AudioManager();
    ~AudioManager();

    static inline std::unordered_map<std::string, Mix_Chunk*> musicTracks; // loaded_audio

    static void PlayMusic(int channel, const std::string& clip_name, bool does_loop) {
        // path could be .wav or .ogg
        int loops = does_loop ? -1 : 0;

        std::string path;
        std::string WavPath = "resources/audio/" + clip_name + ".wav";
        std::string OggPath = "resources/audio/" + clip_name + ".ogg";
        if (std::filesystem::exists(WavPath)) {
            path = WavPath;
        } else if (std::filesystem::exists(OggPath)) {
            path = OggPath;
        } else {
            std::cout << "Audio file not found: " << clip_name << std::endl;
        }

        // load music
        musicTracks[clip_name] = AudioHelper::Mix_LoadWAV498(path.c_str());

        if (musicTracks.find(clip_name) != musicTracks.end()) {
            AudioHelper::Mix_PlayChannel498(channel, musicTracks[clip_name], loops);
        }
    }

    static void StopMusic() {
        AudioHelper::Mix_HaltChannel498(0);
    }

    static void SetVolume(int channel, int volume) {
        AudioHelper::Mix_Volume498(channel, volume);
    }

};

AudioManager::AudioManager() {
    // Initialize audio system
    AudioHelper::Mix_OpenAudio498(44100, MIX_DEFAULT_FORMAT, 2, 2048);
    // channels 0-49 must be available via Mix_allocateChannels()
    AudioHelper::Mix_AllocateChannels498(50);
}

AudioManager::~AudioManager() {
    AudioHelper::Mix_CloseAudio498();
}







#endif //MAIN_CPP_AUDIOMANAGER_H
