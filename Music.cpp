#include<iostream>
#include "Music.hpp"

Music::Music(std::string filepath){
    if(Mix_OpenAudio(44100,MIX_DEFAULT_FORMAT,2,1024) == -1){
        std::cout << "Audo library not working" << std::endl;
    }

    m_music  = Mix_LoadMUS(filepath.c_str());

}

Music::~Music(){
    Mix_FreeMusic(m_music);

}

void Music::SetVolume(int volume) {
    Mix_VolumeMusic(volume);
}

void Music::PlayMusic(int loops){
    if(m_music != nullptr){
        Mix_PlayMusic(m_music,loops);
    }

}


void Music::pauseMusic(){
    Mix_PauseMusic();
}