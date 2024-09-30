#ifndef _MUSIC_HPP_
#define _MUSIC_HPP

#include<string>
#include<SDL2/SDL_mixer.h>

class Music{
public:
    Music(std::string filepath) ;


    ~Music();

    void PlayMusic(int loops);

    void pauseMusic();



    void SetVolume(int volume) ;

private :
    Mix_Music* m_music;

};

#endif