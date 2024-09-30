#include <SDL2/SDL_mixer.h>


class Sound {
public:
    Sound(std::string filepath);
    ~Sound();

    void play(int channel);
    void stop(int channel);

private:
    Mix_Chunk* m_chunk;
};

