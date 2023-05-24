/* Mini SDL Demo
 * featuring SDL2 + SDL2_mixer + SDL2_image + SDL2_ttf
 * on Nintendo Switch using libnx
 *
 * (c) 2018-2021 carstene1ns, ISC license
 */
//Base Vel X = 17
//Base Vel Y = 11

#include <time.h>
#include <unistd.h>
#include <SDL.h>
#include <SDL_mixer.h>
#include <SDL_image.h>
#include <SDL_ttf.h>
#ifdef __SWITCH__
#include <switch.h>
#endif

// some switch buttons
#define JOY_A     0
#define JOY_B     1
#define JOY_X     2
#define JOY_Y     3
#define JOY_MINUS 11
#define JOY_PLUS  10
#define JOY_LEFT  12
#define JOY_UP    13
#define JOY_RIGHT 14
#define JOY_DOWN  15

#define SCREEN_W 1280
#define SCREEN_H 720

SDL_Texture *render_text(SDL_Renderer *renderer, const char *text, TTF_Font *font, SDL_Color color, SDL_Rect *rect) {
    SDL_Surface *surface;
    SDL_Texture *texture;
    surface = TTF_RenderText_Solid(font, text, color);
    texture = SDL_CreateTextureFromSurface(renderer, surface);

    rect->w = surface->w;
    rect->h = surface->h;

    SDL_FreeSurface(surface);

    return texture;
}

int rand_range(int min, int max) {
    return min + rand() / (RAND_MAX / (max - min + 1) + 1);
}

int randomizeBallSpeed(int relMin, int relMax) {
    int vel_x;
    if (rand_range(0,1)) {
        vel_x = rand_range(relMin, relMax);
    } else {
        vel_x = -(rand_range(relMin, relMax));
    }
    return vel_x;
}

int randomizeBallDirection(int relMin, int relMax) {
    int vel_y;
    if (rand_range(0,1)) {
        vel_y = rand_range(relMin, relMax);
    } else {
        vel_y = -(rand_range(relMin, relMax));
    }
    return vel_y;
}

int main(int argc, char** argv) {

    // find our data files
#ifdef __SWITCH__
    romfsInit();
    chdir("romfs:/");
#else
    chdir("romfs");
#endif

    int exit_requested = 0;
    int wait = 25;
    int TextWait = 0;

    SDL_Texture *ball_tex = NULL, *moves_tex = NULL;
    SDL_Rect ball_rect = { 0, 0, 0, 0 };
    SDL_Texture *player1_tex = NULL;
    SDL_Rect player1_rect = { 0, 0, 0, 0 };
    SDL_Texture *player2_tex = NULL;
    SDL_Rect player2_rect = { 0, 0, 0, 0 };
    Mix_Music *music = NULL;
    Mix_Chunk *sound[4] = { NULL };
    SDL_Event event;

    int snd = 0;

    SDL_Color colors[] = {
        { 0, 0, 0, 0 }, // black
        { 255, 255, 255, 0 }, // white
    };

    srand(time(NULL));

    int vel_x = 0;
    int vel_y = 0;
    int autorize_accel = 0;

    bool game_not_started = true;

    bool UP1 = false;
    bool DOWN1 = false;
    bool UP2 = false;
    bool DOWN2 = false;

    SDL_Init(SDL_INIT_VIDEO|SDL_INIT_TIMER);
    Mix_Init(MIX_INIT_OGG);
    IMG_Init(IMG_INIT_PNG);
    TTF_Init();
    
    SDL_Window* window = SDL_CreateWindow("Pong", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, SCREEN_W, SCREEN_H, SDL_WINDOW_SHOWN);
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE);

    SDL_Surface *ball = IMG_Load("data/ball.png");
    SDL_Surface *player1 = IMG_Load("data/player.png");
    SDL_Surface *player2 = IMG_Load("data/player.png");
    if (ball) {
        ball_rect.x = SCREEN_W / 2 - ball->w / 2;
        ball_rect.y = SCREEN_H / 2 - ball->h / 2;
        ball_rect.w = ball->w;
        ball_rect.h = ball->h;
        ball_tex = SDL_CreateTextureFromSurface(renderer, ball);
        SDL_FreeSurface(ball);
    }
    if (player1) {
        player1_rect.x = player1->w + player1->w / 2 + 20;
        player1_rect.y = SCREEN_H / 2 - player1->h / 2;
        player1_rect.w = player1->w;
        player1_rect.h = player1->h;
        player1_tex = SDL_CreateTextureFromSurface(renderer, player1);
        SDL_FreeSurface(player1);
    }
    if (player2) {
        player2_rect.x = SCREEN_W - player2->w - player2->w / 2 -20;
        player2_rect.y = SCREEN_H / 2 - player2->h / 2;
        player2_rect.w = player2->w;
        player2_rect.h = player2->h;
        player2_tex = SDL_CreateTextureFromSurface(renderer, player2);
        SDL_FreeSurface(player2);
    }


#ifdef __SWITCH__
    SDL_InitSubSystem(SDL_INIT_JOYSTICK);
    SDL_JoystickEventState(SDL_ENABLE);
    SDL_JoystickOpen(0);
#endif

    SDL_InitSubSystem(SDL_INIT_AUDIO);
    Mix_AllocateChannels(5);
    Mix_OpenAudio(48000, AUDIO_S16, 2, 4096);

    // load music and sounds from files
    music = Mix_LoadMUS("data/background.ogg");
    sound[0] = Mix_LoadWAV("data/border.wav");
    sound[1] = Mix_LoadWAV("data/collision.wav");
    sound[2] = Mix_LoadWAV("data/dead.wav");

    // load font
    TTF_Font* font = TTF_OpenFont("data/OpenSans-Regular.ttf", 36);

    // render text as texture
    SDL_Rect moves_rect = { 0, 0, 0, 0 };
    moves_tex = render_text(renderer, "UP/ DOWN and X / B to move", font, colors[1], &moves_rect);

    moves_rect.x = SCREEN_W / 2 - moves_rect.w /2;
    moves_rect.y = 100 ;

    // no need to keep the font loaded
    TTF_CloseFont(font);


    while (!exit_requested
#ifdef __SWITCH__
        && appletMainLoop()
#endif
        ) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {exit_requested = 1;}

#ifdef __SWITCH__
            if (event.type == SDL_JOYBUTTONDOWN) {
                if (event.jbutton.button == JOY_PLUS || event.jbutton.button == JOY_MINUS) {
                    exit_requested = 1;
                }
                if (event.jbutton.button == JOY_X) {
                    UP2 = true;
                } else if (event.jbutton.button == JOY_B) {
                    DOWN2 = true;
                } else if (event.jbutton.button == JOY_UP) {
                    UP1 = true;
                } else if (event.jbutton.button == JOY_DOWN) {
                    DOWN1 = true;
                } else if (game_not_started) {
                    vel_y = randomizeBallDirection(9, 13);
                    vel_x = randomizeBallSpeed(15, 19);
                    game_not_started = false;
                }
            }
            if (event.type == SDL_JOYBUTTONUP) {
                if (event.jbutton.button == JOY_X) {
                    UP2 = false;
                }
                if (event.jbutton.button == JOY_B) {
                    DOWN2 = false;
                }
                if (event.jbutton.button == JOY_UP) {
                    UP1 = false;
                }
                if (event.jbutton.button == JOY_DOWN) {
                    DOWN1 = false;
                }
            }

#else
            // use keyboard
            if (event.type == SDL_KEYDOWN) {
                if (event.key.keysym.sym == SDLK_ESCAPE)
                    exit_requested = 1;
            }
#endif
        }

        /* -------------------------------------------------------------------------- */
        /*                                    LOOP                                    */
        /* -------------------------------------------------------------------------- */

        //Move Players
        if (UP1 && player1_rect.y  >= 20) {
            player1_rect.y -= 25;
        }else if (DOWN1 && player1_rect.y + player1_rect.h <= SCREEN_H - 20) {
            player1_rect.y += 25;
        }
        if (UP2 && player2_rect.y  >= 20) {
            player2_rect.y -= 25;
        }else if (DOWN2 && player2_rect.y + player2_rect.h <= SCREEN_H - 20) {
            player2_rect.y += 25;
        }

        //Renvoyer la balle
        if (SDL_HasIntersection(&ball_rect, &player1_rect)) {
            // Collision detected P1!
            if (autorize_accel == 5) {
                vel_x = abs(vel_x) + 1;
                vel_y ++;
                autorize_accel = 0;
            } else {
                vel_x = abs(vel_x);
                autorize_accel ++;
            }
            snd = 1;
            if (sound[snd])
                Mix_PlayChannel(-1, sound[snd], 0);
        }
        if (SDL_HasIntersection(&ball_rect, &player2_rect)) {
            // Collision detected P2!
            if (autorize_accel == 5) {
                vel_x = -1 * (abs(vel_x) + 1);
                vel_y ++;
                autorize_accel = 0;
            } else {
                vel_x = -1 * (abs(vel_x));
                autorize_accel ++;
            }
            snd = 1;
            if (sound[snd])
                Mix_PlayChannel(-1, sound[snd], 0);
        }

        if (TextWait == 51) {TextWait = 0;}
        TextWait ++;

        //mort
        if (game_not_started) {
            if (TextWait < 41) {SDL_SetTextureColorMod(moves_tex, 255,255,255);}
            else {SDL_SetTextureColorMod(moves_tex, 0,0,0);}
        } else {
            SDL_SetTextureColorMod(moves_tex, 0,0,0);
        }


        // set position and bounce on the walls
        ball_rect.y += vel_y;
        ball_rect.x += vel_x;
        // Trop à droite
        if (ball_rect.x + ball_rect.w > SCREEN_W) {
            //Player 2 failed
            game_not_started = true;
            ball_rect.x = SCREEN_W /2 - ball_rect.w /2;
            ball_rect.y = SCREEN_H /2 - ball_rect.h /2;
            vel_x = 0;
            vel_y = 0;
            snd = 2;
            if (sound[snd])
                Mix_PlayChannel(-1, sound[snd], 0);
        }
        // Trop à gauche
        if (ball_rect.x < 0) {
            //Player 1 failed
            game_not_started = true;
            ball_rect.x = SCREEN_W /2 - ball_rect.w /2;
            ball_rect.y = SCREEN_H /2 - ball_rect.h /2;
            vel_x = 0;
            vel_y = 0;
            snd = 2;
            if (sound[snd])
                Mix_PlayChannel(-1, sound[snd], 0);
        }
        // Trop en bas
        if (ball_rect.y + ball_rect.h > SCREEN_H) {
            ball_rect.y = SCREEN_H - ball_rect.h;
            vel_y = -1 * (abs(vel_y));
            snd = 0;
            if (sound[snd])
                Mix_PlayChannel(-1, sound[snd], 0);
        }
        //Trop en haut
        if (ball_rect.y < 0) {
            ball_rect.y = 0;
            vel_y = abs(vel_y);
            snd = 0;
            if (sound[snd])
                Mix_PlayChannel(-1, sound[snd], 0);
        }

        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0xFF);
        SDL_RenderClear(renderer);
        

        
        if (moves_tex){SDL_RenderCopy(renderer, moves_tex, NULL, &moves_rect);}
        if (ball_tex) {SDL_RenderCopy(renderer, ball_tex, NULL, &ball_rect);}
        if (player1_tex) {SDL_RenderCopy(renderer, player1_tex, NULL, &player1_rect);}
        if (player2_tex) {SDL_RenderCopy(renderer, player2_tex, NULL, &player2_rect);}


        SDL_RenderPresent(renderer);

        SDL_Delay(wait); //40 FPS
    }

    // clean up your textures when you are done with them

    if (ball_tex)
        SDL_DestroyTexture(ball_tex);
    if (player1_tex)
        SDL_DestroyTexture(player1_tex);
    if (player2_tex)
        SDL_DestroyTexture(player2_tex);


    // stop sounds and free loaded data
    Mix_HaltChannel(-1);
    Mix_FreeMusic(music);
    for (snd = 0; snd < 4; snd++)
        if (sound[snd])
            Mix_FreeChunk(sound[snd]);

    IMG_Quit();
    Mix_CloseAudio();
    Mix_Quit();
    SDL_Quit();

#ifdef __SWITCH__
    romfsExit();
#endif

    return 0;
}
