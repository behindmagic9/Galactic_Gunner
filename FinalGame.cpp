#include <iostream>
#include <SDL2/SDL.h>
#include <vector>
#include <ctime>
#include <algorithm>
#include <SDL2/SDL_ttf.h>
#include "Sound.hpp"
#include "Music.hpp"

enum GameStates
{
    Menu,
    START,
    PLAYING,
    GAMEOVER,
};

// Sound* CollisionSound;
// Sound* ScoreSound;
// Sound* fire_sound;
// Music* MusicTrack;

struct Fire
{

    SDL_Rect fireRect;
    float fireSpeed;
    bool active;
    int direction; // 1 for right, -1 for left
};

struct Heart
{
    SDL_Rect heartRect;
    bool active;

    Heart(int x, int y) : active(true)
    {
        heartRect = {x, y, 40, 40}; // Set heart size (adjust as needed)
    }

    void fall(float speed)
    {
        heartRect.y += speed; // Move heart down
    }
};

struct Enemy
{
    SDL_Rect enemy_rect;
    bool active;
    float moveSpeed;

    Enemy(int x, int y, int w, int h, float speed) : active(true), moveSpeed(speed)
    {
        enemy_rect = {x, y, w, h};
    }

    bool isCollision(const SDL_Rect &fireRect)
    {
        return SDL_HasIntersection(&enemy_rect, &fireRect);
    }

    void onHit()
    {
        active = false;
    }

    void move()
    {
        enemy_rect.x += moveSpeed;
    }
};

struct Dust
{
    SDL_Rect dustRect;
    float fadeSpeed; // Speed of fading effect
    int lifetime;    // How long the dust lasts
    bool active;     // Whether the dust is active
};

struct Tile
{
    SDL_Rect tile_rect;

    bool isColliding(const SDL_Rect &playerRect)
    {
        return playerRect.x + playerRect.w > tile_rect.x &&
               playerRect.x < tile_rect.x + tile_rect.w &&
               playerRect.y + playerRect.h >= tile_rect.y &&
               playerRect.y + playerRect.h <= tile_rect.y + tile_rect.h;
    }
};

struct PotPlayer
{
    bool isJumping = false;
    bool gameIsRunning = true;
    float jumpSpeed = -20.0f;      // Initial jump speed (upward)
    float gravity = 1.0f;          // Gravity acceleration
    float verticalVelocity = 0.0f; // Velocity along y-axis
    float moveSpeed = 5.0f;        // Horizontal move speed
    int groundLevel = 500;
    float rotationAngle = 0.0f;     // Rotation angle
    float maxRotationAngle = 15.0f; // Max rotation when moving left or right
    int direction = 1;              // 1 for right, -1 for left (movement direction)
    SDL_RendererFlip flip = SDL_FLIP_NONE;
    bool isLanding = false;
    int landingTimer = 0;
    const int landingDuration = 10;
    float scaleFactor = 1.0f;
    bool onTile = false;
    bool isFlashing = false;
    Uint32 flashTimer = 0;
    const Uint32 flashDuration = 1000; // Duration of flash effect in milliseconds
    const Uint32 flashInterval = 100;

    int health = 10;
    int score = 0;

    bool isCollide(const SDL_Rect &enemyrect)
    {
        return SDL_HasIntersection(&Player_rect, &enemyrect);
    }

    SDL_Rect Player_rect;

    void ScoreIncrease() { score++; }

    void decreaseHealth() { health--; }

    void moveRight()
    {
        Player_rect.x += moveSpeed;       // Move right
        direction = 1;                    // Set direction to right
        rotationAngle = maxRotationAngle; // Tilt right
        flip = SDL_FLIP_NONE;
    }

    void Collide(int tiley)
    {

        verticalVelocity = 0;                  // Reset vertical velocity
        Player_rect.y = tiley - Player_rect.h; // Stick player to tile's top surface
        isLanding = false;                     // Player is on the tile
        onTile = true;
        isJumping = false; // Stop jumping since landed
    }

    void NotonTile()
    {
        onTile = false;
        if (!isJumping)
        {
            Player_rect.y += verticalVelocity; // Apply vertical velocity
            verticalVelocity = gravity + 2;
        }
        if (Player_rect.y >= 500)
        {
            Player_rect.y = 500;
        }
    }

    void MoveLeft()
    {
        Player_rect.x -= moveSpeed;        // Move left
        direction = -1;                    // Set direction to left
        rotationAngle = -maxRotationAngle; // Tilt left
        flip = SDL_FLIP_HORIZONTAL;
    }

    void NoRotation()
    {
        rotationAngle = 0;
    }

    void CheckXBounds()
    {
        if (Player_rect.x <= -12)
        {
            Player_rect.x = 1210;
        }
        else if (Player_rect.x > 1210)
        {
            Player_rect.x = -8;
        }
    }

    void ExecuteJump()
    {
        Player_rect.y += verticalVelocity; // Apply vertical velocity
        verticalVelocity += gravity;       // Apply gravity

        if (Player_rect.y >= groundLevel)
        {
            Player_rect.y = groundLevel;
            isJumping = false; // Stop jumping
            isLanding = true;  // Trigger landing effect
            verticalVelocity = 0;
        }
    }
};

Uint32 lastSpawnTime = 0;
Uint32 spawnDelay = 2000;
Uint32 gameStartTime = SDL_GetTicks(); // Record the start time of the game
const Uint32 timeInterval = 5000;
const Uint32 spawnDelayDecreaseAmount = 200; // Amount to decrease spawn delay
const Uint32 minEnemySpawnDelay = 500;

void SpawnEnemy(std::vector<Enemy> &enemies)
{
    Uint32 currentTime = SDL_GetTicks();
    if (currentTime - lastSpawnTime >= spawnDelay)
    {
        int y = (std::rand() % 4) * 200 + 100;
        if (y >= 500)
        {
            y = 500;
        }
        enemies.emplace_back(-10, y, 80, 80, 2.0f);
        lastSpawnTime = currentTime;
    }
}

void UpdateEnemySpawnDelay(Uint32 currentTime)
{
    Uint32 elapsedTime = currentTime - gameStartTime;

    // Check if 5 seconds have passed
    if (elapsedTime >= timeInterval)
    {
        spawnDelay = std::max(minEnemySpawnDelay, spawnDelay - spawnDelayDecreaseAmount);
        gameStartTime += timeInterval; // Reset the timer
    }
}

void RenderText(SDL_Renderer *renderer, const std::string &message, int x, int y, TTF_Font *font)
{
    SDL_Color color = {0x5A, 0xb7, 0x87}; // White color // 5ab787
    SDL_Surface *surfaceMessage = TTF_RenderText_Solid(font, message.c_str(), color);
    SDL_Texture *messageTexture = SDL_CreateTextureFromSurface(renderer, surfaceMessage);

    SDL_Rect messageRect;
    messageRect.x = x;
    messageRect.y = y;
    messageRect.w = surfaceMessage->w;
    messageRect.h = surfaceMessage->h;

    SDL_RenderCopy(renderer, messageTexture, nullptr, &messageRect);

    SDL_FreeSurface(surfaceMessage);
    SDL_DestroyTexture(messageTexture);
}

Uint32 lastHeartSpawnTime = 0;
const Uint32 heartSpawnDelay = 5000; // Time interval to spawn hearts

void SpawnHeart(std::vector<Heart> &hearts)
{
    Uint32 currentTime = SDL_GetTicks();
    if (currentTime - lastHeartSpawnTime >= heartSpawnDelay)
    {
        int x = std::rand() % 1100; // Random x position
        hearts.emplace_back(x, 0);  // Spawn at y = 0
        lastHeartSpawnTime = currentTime;
    }
}

int main()
{
    SDL_Window *window = nullptr;

    if (SDL_Init(SDL_INIT_VIDEO) < 0)
    {
        std::cout << "SDL could not be initialized! SDL Error: " << SDL_GetError() << std::endl;
        return -1;
    }

    if (TTF_Init() == -1)
    {
        std::cout << "Failed to initialize TTF: " << TTF_GetError() << std::endl;
        return -1;
    }
    if (SDL_Init(SDL_INIT_AUDIO) < 0)
    {
        std::cout << "sdl autio no initialsed " << std::endl;
    }

    if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0)
    {
        std::cerr << "SDL_mixer could not initialize! SDL_mixer Error: " << Mix_GetError() << std::endl;
        SDL_Quit();
        return 1;
    }

    window = SDL_CreateWindow("Ninja Animation with Fire and Dust", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1280, 720, SDL_WINDOW_SHOWN);
    if (!window)
    {
        std::cout << "Window could not be created! SDL Error: " << SDL_GetError() << std::endl;
        return -1;
    }

    SDL_Renderer *render = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!render)
    {
        std::cout << "Renderer could not be created! SDL Error: " << SDL_GetError() << std::endl;
        return -1;
    }

    TTF_Font *font = TTF_OpenFont("Fonts/LIVINGBY.TTF", 70);
    if (font == nullptr)
    {
        std::cout << "Failed to load font: " << TTF_GetError() << std::endl;
    }

    // Load images
    SDL_Surface *background_Space = SDL_LoadBMP("images/backgroundSpace.bmp");
    SDL_Surface *RightPerson = SDL_LoadBMP("images/PotPlayerRight3.bmp");
    SDL_SetColorKey(RightPerson, SDL_TRUE, SDL_MapRGB(RightPerson->format, 0xff, 0xff, 0xff));
    SDL_Surface *fireSurface = SDL_LoadBMP("images/Fire.bmp");
    SDL_Surface *dustSurface = SDL_LoadBMP("images/dust1.bmp"); // Load dust texture

    // loading the enemy
    SDL_Surface *enemy_surface = SDL_LoadBMP("images/Enemy.bmp");

    // loading tile surface
    SDL_Surface *tile_surface = SDL_LoadBMP("images/tile.bmp");

    SDL_Surface *heart_surface = SDL_LoadBMP("images/Heart.bmp");

    if (!background_Space || !RightPerson || !fireSurface || !heart_surface || !dustSurface || !tile_surface || !enemy_surface)
    {
        std::cout << "Error loading images! SDL Error: " << SDL_GetError() << std::endl;
        SDL_Quit();
        return -1;
    }

    std::srand(static_cast<unsigned>(std::time(nullptr)));

    // Set transparency for fire and dust
    SDL_SetColorKey(fireSurface, SDL_TRUE, SDL_MapRGB(fireSurface->format, 0xff, 0xff, 0xff));

    // Create textures
    SDL_Texture *background_texture = SDL_CreateTextureFromSurface(render, background_Space);
    SDL_Texture *Right_person = SDL_CreateTextureFromSurface(render, RightPerson);
    SDL_Texture *fire_texture = SDL_CreateTextureFromSurface(render, fireSurface);
    SDL_Texture *dust_texture = SDL_CreateTextureFromSurface(render, dustSurface);
    SDL_Texture *enemy_texture = SDL_CreateTextureFromSurface(render, enemy_surface);

    // tile texture
    SDL_Texture *tile_texture = SDL_CreateTextureFromSurface(render, tile_surface);
    SDL_Texture *heartTexture = SDL_CreateTextureFromSurface(render, heart_surface);
    SDL_FreeSurface(heart_surface);

    // Free surfaces
    SDL_FreeSurface(background_Space);
    SDL_FreeSurface(RightPerson);
    SDL_FreeSurface(fireSurface);
    SDL_FreeSurface(dustSurface);
    SDL_FreeSurface(enemy_surface);

    std::vector<Enemy> enemies;

// background track
    // MusicTrack = new Music("../Assets/Music/guitarchords.mp3");
    // MusicTrack->PlayMusic(-1);

    Sound hurt_sound("Sound/hurt.wav");

    Sound Healthpick_sound("Sound/health.wav");

    Sound fire_sound("Sound/fire.wav");

    SDL_FreeSurface(tile_surface);

    // Define positions and sizes of elements
    SDL_Rect Space_rect = {0, 0, 1280, 720};
    PotPlayer player;
    player.Player_rect = {1280 / 2, 500, 100, 100};

    // Fire and Dust variables
    std::vector<Fire> fires; // List of active fireballs
    float fireSpeed = 10.0f; // Speed at which fireballFires travel

    std::vector<Dust> dustParticles; // List of active dust particles
    const int dustLifetime = 30;     // Lifetime of dust particles

    const Uint8 *state = SDL_GetKeyboardState(NULL); // Get the keyboard state

    // setting up the tiles

    std::vector<Tile> tiles;

    // lifeline heart
    std::vector<Heart> hearts;
    float heartFallSpeed = 2.0f;

    Tile center_bottom_tile;
    center_bottom_tile.tile_rect = {480, 450, 200, 40};
    tiles.push_back(center_bottom_tile);

    Tile upper_left_tile;
    upper_left_tile.tile_rect = {0, 200, 200, 40};
    tiles.push_back(upper_left_tile);

    Tile upper_right_tile;
    upper_right_tile.tile_rect = {1110, 200, 200, 40};
    tiles.push_back(upper_right_tile);

    Tile middle_right_tile;
    middle_right_tile.tile_rect = {0, 400, 200, 40};
    tiles.push_back(middle_right_tile);

    Tile middle_left_tile;
    middle_left_tile.tile_rect = {1110, 400, 200, 40};
    tiles.push_back(middle_left_tile);

    bool gameIsRunning = true;
    GameStates gamestate = Menu;

    while (gameIsRunning)
    {
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            if (event.type == SDL_QUIT)
            {
                gameIsRunning = false;
            }

            if (gamestate == Menu)
            {
                if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_RETURN)
                {
                    gamestate = PLAYING;
                }
            }
            else if (gamestate == GAMEOVER)
            {
                if (event.key.keysym.sym == SDLK_q)
                {
                    gameIsRunning = false;
                }
            }
            else if (gamestate == PLAYING)
            { // Jump handling
                if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_SPACE && !player.isJumping)
                {
                    player.isJumping = true;
                    player.verticalVelocity = player.jumpSpeed; // Start upward velocity
                }

                // Fire handling
                if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_f)
                {
                    fire_sound.play(1);
                    // Create a fireball starting from the center of the player
                    Fire newFire;

                    if (player.direction < 0)
                    {
                        newFire.fireRect.x = player.Player_rect.x - 25;
                    }
                    else
                    {
                        newFire.fireRect.x = player.Player_rect.x + (player.Player_rect.w) - 13;
                    }

                    // newFire.fireRect.x = Right_Playerrect.x + (Right_Playerrect.w ) - 20;
                    newFire.fireRect.y = player.Player_rect.y + (player.Player_rect.h / 2) - 18;
                    // SDL_Rect fireBoundingBox = GetBoundingBox(render,fire_texture);
                    newFire.fireRect.w = 20; // Width of fireball
                    newFire.fireRect.h = 10; // Height of fireball
                    newFire.fireSpeed = fireSpeed;
                    newFire.active = true;
                    newFire.direction = player.direction; // Fire in the current direction

                    // Add the fireball to the list of fires
                    fires.push_back(newFire);
                }
            }
        }

        if (gamestate == PLAYING)
        {
            Uint32 currentTime = SDL_GetTicks();
            UpdateEnemySpawnDelay(currentTime);
            SpawnEnemy(enemies);
            SpawnHeart(hearts);

            // Handle continuous horizontal movement
            if (state[SDL_SCANCODE_LEFT])
            {
                player.MoveLeft(); // Flip character to face left
            }
            else if (state[SDL_SCANCODE_RIGHT])
            {
                player.moveRight(); // Flip character to face right (no flip)
            }
            else
            {
                player.NoRotation(); // Reset rotation if no key is pressed
            }

            player.CheckXBounds();

            // Apply gravity and handle jumping
            if (player.isJumping)
            {
                player.ExecuteJump();
            }

            // Collision with the tile
            for (auto &tile : tiles)
            {
                if (tile.isColliding(player.Player_rect))
                {
                    if (player.verticalVelocity > 0)
                    {
                        player.Collide(tile.tile_rect.y);
                    }
                }
                else if (!player.onTile && !player.isJumping)
                {
                    // If player moves off the tile
                    player.NotonTile();
                }
                else
                {
                    player.onTile = false;
                }
            }

            for (auto &heart : hearts)
            {
                if (heart.active)
                {
                    heart.fall(heartFallSpeed); // Move heart down

                    // Check for collision with player
                    if (player.isCollide(heart.heartRect))
                    {
                        heart.active = false; // Deactivate heart on collision
                        player.health++;      // Increase player health
                        Healthpick_sound.play(1);
                        if (player.health >= 10)
                        {
                            player.health = 10;
                        }
                    }

                    // Remove hearts that fall off screen
                    if (heart.heartRect.y > 720)
                    {
                        heart.active = false;
                    }
                }
                hearts.erase(std::remove_if(hearts.begin(), hearts.end(), [](const Heart &heart)
                                            { return !heart.active; }),
                             hearts.end());
            }

            // Manage dust particles when landing
            if (player.isLanding)
            {
                Dust newDust;
                newDust.dustRect.x = player.Player_rect.x + (player.Player_rect.w / 2) - 25; // Center the dust
                newDust.dustRect.y = player.Player_rect.y + player.Player_rect.h - 20;       // Position dust at ground level
                newDust.dustRect.w = 40;                                                     // Width of dust
                newDust.dustRect.h = 20;                                                     // Height of dust
                newDust.fadeSpeed = 0.1f;                                                    // Fade speed for dust
                newDust.lifetime = dustLifetime;                                             // Set lifetime
                newDust.active = true;                                                       // Activate dust
                dustParticles.push_back(newDust);
                player.isLanding = false; // Reset landing state
            }

            // updating enemy
            for (auto &enemy : enemies)
            {
                if (enemy.active)
                {
                    enemy.move();
                    if (enemy.enemy_rect.x > 1280)
                    {
                        enemy.active = false;
                    }
                }
                if (player.isCollide(enemy.enemy_rect))
                {
                    enemy.active = false;
                    player.decreaseHealth();
                    hurt_sound.play(1);
                    player.isFlashing = true;
                    player.flashTimer = SDL_GetTicks();
                    if (player.health <= 0)
                    {
                        gamestate = GAMEOVER;
                    }
                }
            }

            // Update dust particles
            for (auto &dust : dustParticles)
            {
                if (dust.active)
                {
                    dust.lifetime--; // Decrease lifetime
                    if (dust.lifetime <= 0)
                    {
                        dust.active = false; // Deactivate dust when lifetime ends
                    }
                    dust.dustRect.y -= dust.fadeSpeed; // Optional: rise or fall effect
                    // Optionally change the transparency or size over time here
                }
            }

            // Update fireball positions
            for (auto &fire : fires)
            {
                if (fire.active)
                {
                    fire.fireRect.x += fire.fireSpeed * fire.direction; // Move fireball
                    if (fire.fireRect.x < 0 || fire.fireRect.x > 1280)
                    {
                        fire.active = false; // Deactivate if out of bounds
                    }
                    // player self hurt
                    // if (player.isCollide(fire.fireRect))
                    // {
                    //     player.decreaseHealth(); // Decrease player health
                    //     fire.active = false;     // Fireball hits player
                    // }
                }
            }

            // check for collison between fireball and enemies
            for (auto &fire : fires)
            {
                if (fire.active)
                {
                    for (auto &enemy : enemies)
                    {
                        if (enemy.active && enemy.isCollision(fire.fireRect))
                        {
                            fire.active = false;
                            player.ScoreIncrease();
                            enemy.onHit();
                            break;
                        }
                    }
                }
            }

            enemies.erase(std::remove_if(enemies.begin(), enemies.end(), [](const Enemy &enemy)
                                         { return !enemy.active; }),
                          enemies.end());

            // cleaning the fireballs inactive one
            fires.erase(std::remove_if(fires.begin(), fires.end(), [](const Fire &fire)
                                       { return !fire.active; }),
                        fires.end());
            // cleannign the inactive dust particles
            dustParticles.erase(std::remove_if(dustParticles.begin(), dustParticles.end(), [](const Dust &dust)
                                               { return !dust.active; }),
                                dustParticles.end());

            // Clear the renderer
            SDL_RenderClear(render);
            SDL_SetRenderDrawColor(render, 255, 0, 0, 255);
            SDL_RenderCopy(render, background_texture, NULL, &Space_rect);

            if (player.isFlashing)
            {
                Uint32 currentTime = SDL_GetTicks();
                if (currentTime - player.flashTimer <= player.flashDuration)
                {
                    // Check if we need to toggle the flash state
                    if ((currentTime - player.flashTimer) / player.flashInterval % 2 == 0)
                    {
                        // Render the white texture
                        SDL_SetRenderDrawColor(render, 255, 255, 255, 255); // Set color to white
                        SDL_RenderFillRect(render, &player.Player_rect);    // Draw white rectangle over player
                    }
                    else
                    {
                        // Normal rendering for player
                        SDL_RenderCopyEx(render, Right_person, NULL, &player.Player_rect, player.rotationAngle, NULL, player.flip);
                    }
                }
                else
                {
                    // Reset flashing state
                    player.isFlashing = false;
                }
            }
            else
            {                                                                                                               // Draw background
                SDL_RenderCopyEx(render, Right_person, NULL, &player.Player_rect, player.rotationAngle, NULL, player.flip); // Draw player character
            }
            // rendering tiles
            for (auto &tile : tiles)
            {
                SDL_RenderCopy(render, tile_texture, nullptr, &tile.tile_rect); // Draw the tile
            }

            for (const auto &heart : hearts)
            {
                if (heart.active)
                {
                    // Render heart texture (ensure you have a heart texture loaded)
                    SDL_RenderCopy(render, heartTexture, nullptr, &heart.heartRect);
                }
            }

            // Draw active fireballs
            for (const auto &fire : fires)
            {
                if (fire.active)
                {
                    // SDL_RenderDrawRect(render, &fire.fireRect);
                    SDL_RenderCopy(render, fire_texture, NULL, &fire.fireRect); // Draw fireball
                }
            }

            // rendering the enemies
            for (auto &enemy : enemies)
            {
                if (enemy.active)
                    SDL_RenderCopy(render, enemy_texture, nullptr, &enemy.enemy_rect);
            }

            // Draw active dust particles
            for (const auto &dust : dustParticles)
            {
                if (dust.active)
                {
                    SDL_RenderCopy(render, dust_texture, NULL, &dust.dustRect); // Draw dust
                }
            }

            std::string stringscore = std::to_string(player.score);
            std::string stringHealth = std::to_string(player.health);
            RenderText(render, "Score is :  " + stringscore, 1280 / 2 + 50, 50, font);
            RenderText(render, "Health : " + stringHealth, 100, 50, font);
        }
        else if (gamestate == Menu)
        {
            SDL_RenderClear(render);
            // SDL_RenderCopy
        }
        else if (gamestate == GAMEOVER)
        {
            SDL_RenderClear(render);
        }
        // Present the rendered frame
        SDL_RenderPresent(render);
        SDL_Delay(16); // Maintain ~60 FPS
    }

    // Cleanup
    SDL_DestroyTexture(background_texture);
    SDL_DestroyTexture(Right_person);
    SDL_DestroyTexture(fire_texture);
    SDL_DestroyTexture(dust_texture);
    SDL_DestroyRenderer(render);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
