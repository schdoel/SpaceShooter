#include <stdio.h>
#include <allegro5/allegro.h>
#include <allegro5/allegro_primitives.h>
#include <allegro5/allegro_image.h>
#include <allegro5/allegro_font.h>
#include <allegro5/allegro_ttf.h>
#include <allegro5/allegro_audio.h>
#include <allegro5/allegro_acodec.h>
#include <math.h>

#define LOG_ENABLED
#define MAX_NUM_BULLETS 8
#define MAX_NUM_ENEMIES 16
#define MAX_NUM_BOSSES  8

// Frame rate (frame per second)
const int FPS = 60;
const int SCREEN_W = 1600;
const int SCREEN_H = 900;
const int RESERVE_SAMPLES = 10;
const double SHOOT_COOLDOWN_TIME = 0.2;
const double DAMAGE_COOLDOWN_TIME = 1;
const double BOSS_COOLDOWN_TIME = 2;
const int ENEMY_W = 150;
const int ENEMY_H = 150;
const int PLANE_W = 150;
const int PLANE_H = 150;
const int BOSS_W  = 300;
const int BOSS_H  = 300;
const int vx_dir [] = {0, 1, 1, 1, 0, -1, -1, -1};
const int vy_dir [] = {-1, -1, 0, 1, 1, 1, 0, -1};
enum
{
    SCENE_MENU = 1,
    SCENE_START = 2,
    SCENE_SETTINGS = 3,
    SCENE_LOSE = 4,
    SCENE_WIN = 5,
    SCENE_BOSS = 6
};

/* Input states */

// The active scene id.
int active_scene;
// Keyboard state, whether the key is down or not.
bool key_state[ALLEGRO_KEY_MAX];
// Mouse state, whether the key is down or not.
// 1 is for left, 2 is for right, 3 is for middle.
bool *mouse_state;
// Mouse position.
int mouse_x, mouse_y;

int selected_plane_id;
int selected_bullet_id;
int selected_level_id;

int num_enemies;
double begin_time;
double end_time;

float YY = 0.0;

/* Variables for allegro basic routines. */

ALLEGRO_DISPLAY* game_display;
ALLEGRO_EVENT_QUEUE* game_event_queue;
ALLEGRO_TIMER* game_update_timer;

/* Shared resources*/
ALLEGRO_FONT* font_pirulen_64;
ALLEGRO_FONT* font_pirulen_48;
ALLEGRO_FONT* font_pirulen_32;

/* Menu Scene resources*/
ALLEGRO_BITMAP* img_scene_menu_background;
ALLEGRO_BITMAP* img_settings_button_off;
ALLEGRO_BITMAP* img_settings_button_on;
ALLEGRO_SAMPLE* audio_scene_menu_background;
ALLEGRO_SAMPLE_ID audio_scene_menu_background_id;

/* Start Scene resources*/
ALLEGRO_BITMAP* img_scene_start_background;
ALLEGRO_BITMAP* img_scene_start_boss_background;
ALLEGRO_BITMAP* img_scene_lose_background;
ALLEGRO_BITMAP* img_scene_win_background;
ALLEGRO_BITMAP* img_plane[3];
ALLEGRO_BITMAP* img_bullet[3];
ALLEGRO_BITMAP* img_enemy;
ALLEGRO_BITMAP* img_boss;
ALLEGRO_BITMAP* img_boss_copy;
ALLEGRO_BITMAP* img_heart;
ALLEGRO_SAMPLE* audio_scene_start_background;
ALLEGRO_SAMPLE_ID audio_scene_start_background_id;

typedef struct
{
    float x, y; // center
    float w, h;
    float vx, vy;
    int dir;
    bool hidden;
    int hp;
    int lives;
    double last_shoot_timestamp;
    double last_damage_timestamp;
    ALLEGRO_BITMAP *img;
}
MovableObject;

void draw_movable_object (MovableObject obj);
MovableObject plane;
MovableObject enemies[MAX_NUM_ENEMIES];
MovableObject bullets[3][MAX_NUM_BULLETS];
MovableObject boss;
MovableObject boss_copy[MAX_NUM_BOSSES];

/* Declare function prototypes. */

// Initialize allegro5 library
void allegro5_init(void);
// Initialize variables and resources.
// Allows the game to perform any initialization it needs before
// starting to run.
void game_init(void);
// Process events inside the event queue using an infinity loop.
void game_start_event_loop(void);
// Run game logic such as updating the world, checking for collision,
// switching scenes and so on.
// This is called when the game should update its logic.
void game_update(void);
// Draw to display.
// This is called when the game should draw itself.
void game_draw(void);
// Release resources.
// Free the pointers we allocated.
void game_destroy(void);
// Function to change from one scene to another.
void game_change_scene (int next_scene);
// Load resized bitmap and check if failed.
ALLEGRO_BITMAP *load_bitmap_resized(const char *filename, int w, int h);
// [HACKATHON 3-2]
// TODO: Declare a function.
// Determines whether the point (px, py) is in rect (x, y, w, h).
bool pnt_in_rect(int px, int py, int x, int y, int w, int h);
bool collision (float x1, float y1, float w1, float h1, float x2, float y2, float w2, float h2);
bool is_hit_boundary (float x, float y, float w, float h);

/* Event callbacks. */
void on_key_down(int keycode);
void on_mouse_down(int btn, int x, int y);

/* Declare function prototypes for debugging. */

// Display error message and exit the program, used like 'printf'.
// Write formatted output to stdout and file from the format string.
// If the program crashes unexpectedly, you can inspect "log.txt" for
// further information.
void game_abort(const char* format, ...);
// Log events for later debugging, used like 'printf'.
// Write formatted output to stdout and file from the format string.
// You can inspect "log.txt" for logs in the last run.
void game_log(const char* format, ...);
// Log using va_list.
void game_vlog(const char* format, va_list arg);

int main (int argc, char** argv)
{
    // Set random seed for better random outcome.
    srand((unsigned)time(NULL));
    allegro5_init();
    game_log("Allegro5 initialized");
    game_log("Game begin");
    // Initialize game variables.
    game_init();
    game_log("Game initialized");
    // Draw the first frame.
    game_draw();
    game_log("Game start event loop");
    // This call blocks until the game is finished.
    game_start_event_loop();
    game_log("Game end");
    game_destroy();
    return 0;
}

void allegro5_init (void)
{
    if (!al_init())
        game_abort("failed to initialize allegro");

    // Initialize add-ons.
    if (!al_init_primitives_addon())
        game_abort("failed to initialize primitives add-on");
    if (!al_init_font_addon())
        game_abort("failed to initialize font add-on");
    if (!al_init_ttf_addon())
        game_abort("failed to initialize ttf add-on");
    if (!al_init_image_addon())
        game_abort("failed to initialize image add-on");
    if (!al_install_audio())
        game_abort("failed to initialize audio add-on");
    if (!al_init_acodec_addon())
        game_abort("failed to initialize audio codec add-on");
    if (!al_reserve_samples(RESERVE_SAMPLES))
        game_abort("failed to reserve samples");
    if (!al_install_keyboard())
        game_abort("failed to install keyboard");
    if (!al_install_mouse())
        game_abort("failed to install mouse");
    // TODO: Initialize other addons such as video, ...

    // Setup game display.
    game_display = al_create_display(SCREEN_W, SCREEN_H);
    if (!game_display)
        game_abort("failed to create display");
    al_set_window_title(game_display, "I2P(I)_2020 Final Project <109006205>");

    // Setup update timer.
    game_update_timer = al_create_timer(1.0 / FPS);
    if (!game_update_timer)
        game_abort("failed to create timer");

    // Setup event queue.
    game_event_queue = al_create_event_queue();
    if (!game_event_queue)
        game_abort("failed to create event queue");

    // Malloc mouse buttons state according to button counts.
    const unsigned m_buttons = al_get_mouse_num_buttons();
    game_log("There are total %u supported mouse buttons", m_buttons);
    // mouse_state[0] will not be used.
    mouse_state = malloc((m_buttons + 1) * sizeof(bool));
    memset(mouse_state, false, (m_buttons + 1) * sizeof(bool));

    // Register display, timer, keyboard, mouse events to the event queue.
    al_register_event_source(game_event_queue, al_get_display_event_source(game_display));
    al_register_event_source(game_event_queue, al_get_timer_event_source(game_update_timer));
    al_register_event_source(game_event_queue, al_get_keyboard_event_source());
    al_register_event_source(game_event_queue, al_get_mouse_event_source());
    al_start_timer(game_update_timer);
}

void game_init (void)
{
    /* Fonts */
    font_pirulen_64 = al_load_font("pirulen.ttf", 64, 0);
    if (!font_pirulen_64)
    {
        game_abort("failed to load font: pirulen.ttf with size 64");
    }
    font_pirulen_48 = al_load_font("pirulen.ttf", 48, 0);
    if (!font_pirulen_48)
    {
        game_abort("failed to load font: pirulen.ttf with size 48");
    }
    font_pirulen_32 = al_load_font("pirulen.ttf", 32, 0);
    if (!font_pirulen_32)
    {
        game_abort("failed to load font: pirulen.ttf with size 32");
    }

    /* Menu Scene resources main-bg.jpg */
    img_scene_menu_background = load_bitmap_resized("scene_menu_background.png", SCREEN_W, SCREEN_H);

    audio_scene_menu_background = al_load_sample("scene_menu_background.ogg");
    if (!audio_scene_menu_background)
    {
        game_abort("failed to load audio: scence_menu_background.ogg");
    }
    
    /* Scene start Background*/
    audio_scene_start_background = al_load_sample("scene_start_background.ogg");
    if (!audio_scene_start_background)
    {
        game_abort("failed to load audio: scene_start_background.ogg");
    }
    
    /* Setting Button*/
    img_settings_button_off = load_bitmap_resized("settings_button_off.png", 100, 100);
    if (!img_settings_button_off)
    {
        game_abort("failed to load image: settings_button_off.png");
    }
    img_settings_button_on = load_bitmap_resized("settings_button_on.png", 100, 100);
    if (!img_settings_button_on)
    {
        game_abort("failed to load image: settings_button_on.png");
    }

    /* Start Scene resources*/
    img_scene_start_background = al_load_bitmap("scene_start_background1.png");
    //img_scene_start_background = load_bitmap_resized("scene_start_background.jpg", SCREEN_W, SCREEN_H);
    
    char img_plane_path [] = "plane/?.png";
    for (int i = 0; i < 3; ++i)
    {
        img_plane_path[6] = ('0' + i);
        img_plane[i] = al_load_bitmap(img_plane_path);
        if (!img_plane[i])
        {
            game_abort("failed to load image: ", img_plane_path);
        }
    }

    img_enemy = load_bitmap_resized("enemy.png", ENEMY_W, ENEMY_H);
    if (!img_enemy)
    {
        game_abort("failed to load image: enemy.png");
    }
    
    img_boss = load_bitmap_resized("boss.png", BOSS_W, BOSS_H);
    if (!img_boss)
    {
        game_abort("failed to load image: boss.png");
    }
    
    img_boss_copy = load_bitmap_resized("boss.png", BOSS_W / 2, BOSS_H / 2);
    if(!img_boss_copy)
    {
        game_abort("failed to laod image: boss_copy.png");
    }

    /*Bullets */
    char img_bullet_path [] = "bullet/?.png";
    for (int i = 0; i < 3; ++i)
    {
        img_bullet_path[7] = ('0' + i);
        img_bullet[i] = load_bitmap_resized(img_bullet_path, 20, 40);
        if (!img_bullet[i])
        {
            game_abort("failed to load image: ", img_bullet_path);
        }
    }

    img_scene_lose_background = al_load_bitmap("scene_lose_background_3.png");
    if (!img_scene_lose_background)
    {
        game_abort("failed to load image: img_scene_lose_background");
    }

    img_scene_win_background = load_bitmap_resized("scene_win_background.png",SCREEN_W,SCREEN_H);
    if(!img_scene_win_background)
    {
        game_abort("failed to load image: scene_win_background.png");
    }
    
    img_heart = load_bitmap_resized("heart.png", 70, 70);
    if (!img_heart)
    {
        game_abort("failed to load image: heart.png");
    }
    game_change_scene(SCENE_MENU);
    
    return;
}

void game_start_event_loop (void)
{
    bool done = false;
    ALLEGRO_EVENT event;
    int redraws = 0;
    while (!done)
    {
        al_wait_for_event(game_event_queue, &event);
        if (event.type == ALLEGRO_EVENT_DISPLAY_CLOSE)
        {
            // Event for clicking the window close button.
            game_log("Window close button clicked");
            done = true;
        }
        else if (event.type == ALLEGRO_EVENT_TIMER)
        {
            // Event for redrawing the display.
            if (event.timer.source == game_update_timer)
            {
                // The redraw timer has ticked.
                redraws++;
            }
            //velocity of scene_start_background
            YY+=0.7;
        }
        else if (event.type == ALLEGRO_EVENT_KEY_DOWN)
        {
            // Event for keyboard key down.
            game_log("Key with keycode %d down", event.keyboard.keycode);
            key_state[event.keyboard.keycode] = true;
            on_key_down(event.keyboard.keycode);
        }
        else if (event.type == ALLEGRO_EVENT_KEY_UP)
        {
            // Event for keyboard key up.
            game_log("Key with keycode %d up", event.keyboard.keycode);
            key_state[event.keyboard.keycode] = false;
        }
        else if (event.type == ALLEGRO_EVENT_MOUSE_BUTTON_DOWN)
        {
            // Event for mouse key down.
            game_log("Mouse button %d down at (%d, %d)", event.mouse.button, event.mouse.x, event.mouse.y);
            mouse_state[event.mouse.button] = true;
            on_mouse_down(event.mouse.button, event.mouse.x, event.mouse.y);
        }
        else if (event.type == ALLEGRO_EVENT_MOUSE_BUTTON_UP)
        {
            // Event for mouse key up.
            game_log("Mouse button %d up at (%d, %d)", event.mouse.button, event.mouse.x, event.mouse.y);
            mouse_state[event.mouse.button] = false;
        }
        else if (event.type == ALLEGRO_EVENT_MOUSE_AXES)
        {
            if (event.mouse.dx != 0 || event.mouse.dy != 0)
            {
                // Event for mouse move.
                // game_log("Mouse move to (%d, %d)", event.mouse.x, event.mouse.y);
                mouse_x = event.mouse.x;
                mouse_y = event.mouse.y;
            }
            else if (event.mouse.dz != 0)
            {
                // Event for mouse scroll.
                game_log("Mouse scroll at (%d, %d) with delta %d", event.mouse.x, event.mouse.y, event.mouse.dz);
            }
        }
        // entries inside Scene.
        if (key_state[ALLEGRO_KEY_ESCAPE])
        {
            game_log("press escape key to quit");
            done = true;
        }
        else
        {
            // Redraw
            if (redraws > 0 && al_is_event_queue_empty(game_event_queue))
            {
                // if (redraws > 1)
                //     game_log("%d frame(s) dropped", redraws - 1);
                // Update and draw the next frame.
                game_update();
                game_draw();
                redraws = 0;
            }
        }
    }
    return;
}

void game_update (void)
{
    if (active_scene == SCENE_START)
    {
        if(YY>900) YY = 0.0;
        end_time = al_get_time();
        plane.vx = plane.vy = 0;
       
        if (key_state[ALLEGRO_KEY_LEFT]){
            plane.vx = -1;
            plane.dir = 6;
        }
        
        if (key_state[ALLEGRO_KEY_RIGHT]){
            plane.vx = 1;
            plane.dir = 2;
        }
        
        if (key_state[ALLEGRO_KEY_UP]){
            plane.vy = -1;
            plane.dir = 0;
        }
        
        if (key_state[ALLEGRO_KEY_DOWN]){
            plane.vy = 1;
            plane.dir = 4;
        }
        // 0.71 is (1/sqrt(2))
        const float ratio = 0.707106f;
        const float plane_step = 5.0f;
        const float bullet_step = 10.0f;
        plane.x += plane.vx * (plane.vy ? ratio : 1) * plane_step;
        plane.y += plane.vy * (plane.vx ? ratio : 1) * plane_step;
        
        if (key_state[ALLEGRO_KEY_UP]){
            
            if (key_state[ALLEGRO_KEY_RIGHT]){
                plane.dir = 1;
            }
            
            else if (key_state[ALLEGRO_KEY_LEFT]){
                plane.dir = 7;
            }
        }
        
        else if (key_state[ALLEGRO_KEY_DOWN]){
            
            if (key_state[ALLEGRO_KEY_RIGHT]){
                plane.dir = 3;
            }
            
            else if (key_state[ALLEGRO_KEY_LEFT]){
                plane.dir = 5;
            }
        }

        // TODO: Limit the plane's collision box inside the frame.
        if (plane.x - (plane.w / 2) < 0){
            plane.x = (plane.w / 2);
        }
        
        else if (plane.x + (plane.w / 2) > SCREEN_W){
            plane.x = SCREEN_W - (plane.w / 2);
        }
        
        if (plane.y - (plane.h / 2) < 0){
            plane.y = plane.h / 2;
        }
        
        else if (plane.y + (plane.h / 2) > SCREEN_H){
            plane.y = SCREEN_H - (plane.h / 2);
        }

        // TODO: Update bullet coordinates.
        for (int i = 0; i < MAX_NUM_BULLETS; ++i)
        {
            if (!bullets[selected_bullet_id][i].hidden)
            {
                bullets[selected_bullet_id][i].x
                += bullets[selected_bullet_id][i].vx * ((bullets[selected_bullet_id][i].vy) ? ratio : 1) * bullet_step;
                bullets[selected_bullet_id][i].y
                += bullets[selected_bullet_id][i].vy * ((bullets[selected_bullet_id][i].vx) ? ratio : 1) * bullet_step;
            }
        }

        // TODO: Shoot if key is down and cool-down is over.
        if( key_state[ALLEGRO_KEY_SPACE] && ((al_get_time() - plane.last_shoot_timestamp) > SHOOT_COOLDOWN_TIME))
        {
            for (int i = 0; i < MAX_NUM_BULLETS; i++)
            {
                if (bullets[selected_bullet_id][i].hidden)
                {
                    plane.last_shoot_timestamp = al_get_time();
                    bullets[selected_bullet_id][i].x = plane.x;
                    bullets[selected_bullet_id][i].y = plane.y;
                    bullets[selected_bullet_id][i].dir = plane.dir;
                    bullets[selected_bullet_id][i].vx = vx_dir[plane.dir];
                    bullets[selected_bullet_id][i].vy = vy_dir[plane.dir];
                    bullets[selected_bullet_id][i].hidden = false;
                    break;
                }
            }
        }
        //if bullets not hidden and hit the boundary, make it hidden and hide it
        for (int i = 0; i < MAX_NUM_BULLETS; i++)
        {
            if((!bullets[selected_bullet_id][i].hidden) && is_hit_boundary(
                 bullets[selected_bullet_id][i].x,
                 bullets[selected_bullet_id][i].y,
                 bullets[selected_bullet_id][i].w,
                 bullets[selected_bullet_id][i].h
                )
               )
            {
                bullets[selected_bullet_id][i].x = -bullets[selected_bullet_id][i].w;
                bullets[selected_bullet_id][i].x = -bullets[selected_bullet_id][i].h;
                bullets[selected_bullet_id][i].hidden = true;
            }
        }
        
        //if there is collision between bullets and enemy, enemy lives decrease, if lives == 0, make enemy hidden
        for (int i = 0; i < MAX_NUM_BULLETS; ++i)
        {
            for (int j = 0; j < num_enemies; ++j)
            {
                if(collision(
                        bullets[selected_bullet_id][i].x,
                        bullets[selected_bullet_id][i].y,
                        bullets[selected_bullet_id][i].w,
                        bullets[selected_bullet_id][i].h,
                        enemies[j].x,
                        enemies[j].y,
                        enemies[j].w,
                        enemies[j].h
                    )
                )
                {
                    bullets[selected_bullet_id][i].x = -bullets[selected_bullet_id][i].w;
                    bullets[selected_bullet_id][i].y = -bullets[selected_bullet_id][i].h;
                    bullets[selected_bullet_id][i].hidden = true;
                    --enemies[j].lives;
                    if (enemies[j].lives == 0)
                    {
                        enemies[j].x = -enemies[j].w;
                        enemies[j].y = -enemies[j].h;
                        enemies[j].hidden = true;
                    }
                }
            }
        }

        //if enemies not hidden and it hits boundary, make it go to a different random direction
        for (int i = 0; i < num_enemies; i++)
        {
            if (!enemies[i].hidden)
            {
                if (is_hit_boundary(enemies[i].x, enemies[i].y, enemies[i].w, enemies[i].h))
                {
                    enemies[i].dir = (enemies[i].dir + 4) % 8; //the direction of enemy
                    enemies[i].vx = vx_dir[enemies[i].dir] * (1 + rand() % 5); //equation for the direction of enemies
                    enemies[i].vy = vy_dir[enemies[i].dir] * (1 + rand() % 5);
                    if (enemies[i].x - (enemies[i].w / 2) < 0) //if enemies out of boundary, make it back in
                    {
                        enemies[i].x = (enemies[i].w / 2);
                    }
                    if (enemies[i].x + (enemies[i].w / 2) > SCREEN_W)
                    {
                        enemies[i].x = SCREEN_W - (enemies[i].w / 2);
                    }
                    if (enemies[i].y - (enemies[i].h / 2) < 0)
                    {
                        enemies[i].y = (enemies[i].h / 2);
                    }
                    if (enemies[i].y + (enemies[i].h / 2) > SCREEN_H)
                    {
                        enemies[i].y = SCREEN_H - (enemies[i].y / 2);
                    }
                }
                //updating location by the speed
                enemies[i].x += enemies[i].vx;
                enemies[i].y += enemies[i].vy;
            }
        }
        //if enemies not dead, if more than damage_cooldown_time and colision between enemies and plane
        for (int i = 0; i < num_enemies; i++)
        {
            if (!enemies[i].hidden)
            {
                //kalo mau enemies serang harus lebih > cooldown time, intervalnya
                if(((al_get_time() - plane.last_damage_timestamp) > DAMAGE_COOLDOWN_TIME) &&
                    collision (
                        enemies[i].x,
                        enemies[i].y,
                        enemies[i].w,
                        enemies[i].h,
                        plane.x,
                        plane.y,
                        plane.w,
                        plane.h
                    )
                )
                {
                    plane.last_damage_timestamp = al_get_time(); //store the last damage
                    int damage_value = 10 + (rand() % 30);
                    if (plane.hp - damage_value <= 0)
                    {
                        if (plane.lives > 0)
                        {
                            plane.hp = 100;
                            --(plane.lives);
                        }
                        else
                        {
                            game_change_scene(SCENE_LOSE);
                        }
                    }
                    else
                    {
                        plane.hp -= damage_value;
                    }
                }
            }
            //kalo enemies mati semua, change scene to boss
            bool is_all_dead = true;
            for (int i = 0; i < num_enemies; ++i)
            {
                is_all_dead &= (enemies[i].hidden);
            }
            if (is_all_dead)
            {
                game_change_scene(SCENE_BOSS);
            }
        }
    }
    else if (active_scene == SCENE_BOSS)
    {
        if(YY>900) YY = 0.0; //this is for moving the background picture back to 0 after it goes out of frame
        end_time = al_get_time();
        const float ratio = 0.707106f; //nilai cosin
        const float plane_step = 5.0f; //velocity
        const float bullet_step = 10.0f; //velocity
        const float boss_step = 5.0f; //velocity
        const float boss_copy_step = 10.0f; //velocity
        plane.vx = plane.vy = 0;
        if (key_state[ALLEGRO_KEY_LEFT])
        {
            plane.vx = -1;
            plane.dir = 6;
        }
        if (key_state[ALLEGRO_KEY_RIGHT])
        {
            plane.vx = 1;
            plane.dir = 2;
        }
        if (key_state[ALLEGRO_KEY_UP])
        {
            plane.vy = -1;
            plane.dir = 0;
        }
        if (key_state[ALLEGRO_KEY_DOWN])
        {
            plane.vy = 1;
            plane.dir = 4;
        }
        plane.x += plane.vx * (plane.vy ? ratio : 1) * plane_step; //change the plane position according to the direction 8 directions
        plane.y += plane.vy * (plane.vx ? ratio : 1) * plane_step;
        
        //iNI DIRECTION KEYNYA
        if (key_state[ALLEGRO_KEY_UP])
        {
            if (key_state[ALLEGRO_KEY_RIGHT])
            {
                plane.dir = 1;
            }
            else if (key_state[ALLEGRO_KEY_LEFT])
            {
                plane.dir = 7;
            }
        }
        else if (key_state[ALLEGRO_KEY_DOWN])
        {
            if (key_state[ALLEGRO_KEY_RIGHT])
            {
                plane.dir = 3;
            }
            else if (key_state[ALLEGRO_KEY_LEFT])
            {
                plane.dir = 5;
            }
        }
        
        //boundariesnya
        if (plane.x - (plane.w / 2) < 0)
        {
            plane.x = (plane.w / 2);
        }
        else if (plane.x + (plane.w / 2) > SCREEN_W)
        {
            plane.x = SCREEN_W - (plane.w / 2);
        }
        if (plane.y - (plane.h / 2) < 0)
        {
            plane.y = plane.h / 2;
        }
        else if (plane.y + (plane.h / 2) > SCREEN_H)
        {
            plane.y = SCREEN_H - (plane.h / 2);
        }
        //make the bullets direction same as the plane
        for (int i = 0; i < MAX_NUM_BULLETS; ++i)
        {
            if (!bullets[selected_bullet_id][i].hidden)
            {
                bullets[selected_bullet_id][i].x
                += bullets[selected_bullet_id][i].vx * ((bullets[selected_bullet_id][i].vy) ? ratio : 1) * bullet_step;
                bullets[selected_bullet_id][i].y
                += bullets[selected_bullet_id][i].vy * ((bullets[selected_bullet_id][i].vx) ? ratio : 1) * bullet_step;
            }
        }
        if( key_state[ALLEGRO_KEY_SPACE] && ((al_get_time() - plane.last_shoot_timestamp) > SHOOT_COOLDOWN_TIME) )
        {
            for (int i = 0; i < MAX_NUM_BULLETS; i++)
            {
                if (bullets[selected_bullet_id][i].hidden)
                {
                    plane.last_shoot_timestamp = al_get_time();
                    bullets[selected_bullet_id][i].x = plane.x;
                    bullets[selected_bullet_id][i].y = plane.y;
                    bullets[selected_bullet_id][i].dir = plane.dir;
                    bullets[selected_bullet_id][i].vx = vx_dir[plane.dir];
                    bullets[selected_bullet_id][i].vy = vy_dir[plane.dir];
                    bullets[selected_bullet_id][i].hidden = false;
                    break;
                }
            }
        }
        for (int i = 0; i < MAX_NUM_BULLETS; i++)
        {
            if ((!bullets[selected_bullet_id][i].hidden) &&
                is_hit_boundary(
                 bullets[selected_bullet_id][i].x,
                 bullets[selected_bullet_id][i].y,
                 bullets[selected_bullet_id][i].w,
                 bullets[selected_bullet_id][i].h
                )
            )
            {
                bullets[selected_bullet_id][i].x = -bullets[selected_bullet_id][i].w;
                bullets[selected_bullet_id][i].x = -bullets[selected_bullet_id][i].h;
                bullets[selected_bullet_id][i].hidden = true;
            }
        }
        for (int i = 0; i < MAX_NUM_BULLETS; ++i)
        {
            if(collision(
                    bullets[selected_bullet_id][i].x,
                    bullets[selected_bullet_id][i].y,
                    bullets[selected_bullet_id][i].w,
                    bullets[selected_bullet_id][i].h,
                    boss.x,
                    boss.y,
                    boss.w,
                    boss.h
                )
            )
            {
                bullets[selected_bullet_id][i].x = -bullets[selected_bullet_id][i].w;
                bullets[selected_bullet_id][i].y = -bullets[selected_bullet_id][i].h;
                bullets[selected_bullet_id][i].hidden = true;
                --boss.lives;
                if (boss.lives == 0)
                {
                    boss.x = -boss.w;
                    boss.y = -boss.h;
                    boss.hidden = true;
                    game_change_scene(SCENE_WIN);
                }
            }
        }
        
        if (!boss.hidden)
        {
            if (is_hit_boundary(boss.x, boss.y, boss.w, boss.h))
            {
                boss.dir = (boss.dir + 4) % 8;
                boss.vx = vx_dir[boss.dir] * boss_step;
                boss.vy = vy_dir[boss.dir] * boss_step;
                if (boss.x - (boss.w / 2) < 0)
                {
                    boss.x = (boss.w / 2);
                }
                if (boss.x + (boss.w / 2) > SCREEN_W)
                {
                    boss.x = SCREEN_W - (boss.w / 2);
                }
                if (boss.y - (boss.h / 2) < 0)
                {
                    boss.y = (boss.h / 2);
                }
                if (boss.y + (boss.h / 2) > SCREEN_H)
                {
                    boss.y = SCREEN_H - (boss.y / 2);
                }
            }
            boss.x += boss.vx * (boss.vy ? ratio : 1) * boss_step;
            boss.y += boss.vy * (boss.vx ? ratio : 1) * boss_step;
        }
        
        if ((al_get_time() - boss.last_shoot_timestamp) > BOSS_COOLDOWN_TIME)
        {
            boss.last_shoot_timestamp = al_get_time();
            for (int i = 0; i < MAX_NUM_BOSSES; ++i)
            {
                boss_copy[i].x = boss.x;
                boss_copy[i].y = boss.y;
                boss_copy[i].hidden = false;
            }
        }
        for (int i = 0; i < MAX_NUM_BOSSES; ++i)
        {
            if (!boss_copy[i].hidden)
            {
                if (is_hit_boundary(boss_copy[i].x, boss_copy[i].y, boss_copy[i].w, boss_copy[i].h))
                {
                    boss_copy[i].x = -boss_copy[i].w;
                    boss_copy[i].y = -boss_copy[i].h;
                    boss_copy[i].hidden = false;
                }
                else if
                (
                    ((al_get_time() - plane.last_damage_timestamp) > DAMAGE_COOLDOWN_TIME) &&
                    collision(
                        boss_copy[i].x,
                        boss_copy[i].y,
                        boss_copy[i].w,
                        boss_copy[i].h,
                        plane.x,
                        plane.y,
                        plane.w,
                        plane.h
                    )
                )
                {
                    plane.last_damage_timestamp = al_get_time();
                    int damage_value = 10 + (rand() % 90);
                    if (plane.hp - damage_value <= 0)
                    {
                        if (plane.lives > 0)
                        {
                            plane.hp = 100;
                            --(plane.lives);
                        }
                        else
                        {
                            game_change_scene(SCENE_LOSE);
                        }
                    }
                    else
                    {
                        plane.hp -= damage_value;
                    }
                }
                boss_copy[i].x += boss_copy[i].vx * (boss_copy[i].vy ? ratio : 1) * boss_copy_step;
                boss_copy[i].y += boss_copy[i].vy * (boss_copy[i].vx ? ratio : 1) * boss_copy_step;
            }
        }
    }
    return;
}

void game_draw (void)
{
    if (active_scene == SCENE_MENU)
    {
        al_draw_bitmap(img_scene_menu_background, 0, 0, 0);
        // [HACKATHON 3-5]
        // TODO: Draw settings images.
        if (pnt_in_rect(mouse_x, mouse_y, 1470, 30, 100, 100))
        {
            al_draw_bitmap(img_settings_button_on, 1470, 30, 0);
        }
        else
        {
            al_draw_bitmap(img_settings_button_off, 1470, 30, 0);
        }
    }
    else if (active_scene == SCENE_START)
    {
        if (SCREEN_H + YY <= 900) {
            al_draw_bitmap_region(img_scene_start_background, 0, YY, SCREEN_W, SCREEN_H, 0, 0, 0);
        }
        else
        {
            al_draw_bitmap_region(img_scene_start_background, 0, YY, SCREEN_W, 900 - YY, 0, 0, 0);
            al_draw_bitmap_region(img_scene_start_background, 0, 0, SCREEN_W, SCREEN_H - (900 - YY), 0, 900 - YY, 0);
        }
        //al_draw_bitmap(img_scene_start_background, 0, 0, 0);
        draw_movable_object(plane);
        for (int i = 0; i < num_enemies; ++i)
        {
            draw_movable_object(enemies[i]);
        }
        // [HACKATHON 2-9]
        // TODO: Draw all bullets in your bullet array.
        for (int i = 0; i < MAX_NUM_BULLETS; ++i)
        {
            draw_movable_object(bullets[selected_bullet_id][i]);
        }
        al_draw_rectangle(550, 50, 50, 120, al_map_rgb(150, 150, 150), 5);
        al_draw_filled_rectangle(60 + ((540 - 60) * plane.hp / 100.0), 60, 60, 110, al_map_rgb(255, 0, 0));
        al_draw_filled_rectangle(540, 60, 60 + ((540 - 60) * plane.hp / 100.0), 110, al_map_rgb(100, 0, 0));
        for (int i = 0; i < plane.lives; ++i)
        {
            al_draw_bitmap(img_heart, 560 + (40 * i), 60, 0);
        }
        al_draw_textf(font_pirulen_32, al_map_rgb(255, 255, 255), 310, 65, ALLEGRO_ALIGN_CENTRE, "%d%c", plane.hp, '\%');
        al_draw_rectangle(1550, 50, 1350, 125, al_map_rgb(255, 255, 255), 10);
        int duration = (int)(end_time - begin_time);
        al_draw_textf
        (
            font_pirulen_48, al_map_rgb(255, 255, 255), 1450, 60, ALLEGRO_ALIGN_CENTRE,
            "%02d%c%02d", (duration / 60), ':', (duration % 60)
        );
    }
    else if (active_scene == SCENE_BOSS)
    {
        if (SCREEN_H + YY <= 900) {
            al_draw_bitmap_region(img_scene_start_background, 0, YY, SCREEN_W, SCREEN_H, 0, 0, 0);
        }
        else
        {
            al_draw_bitmap_region(img_scene_start_background, 0, YY, SCREEN_W, 900 - YY, 0, 0, 0);
            al_draw_bitmap_region(img_scene_start_background, 0, 0, SCREEN_W, SCREEN_H - (900 - YY), 0, 900 - YY, 0);
        }
        //al_draw_bitmap(img_scene_start_background, 0, 0, 0);
        draw_movable_object(plane);
        draw_movable_object(boss);
        for (int i = 0; i < MAX_NUM_BOSSES; ++i)
        {
            draw_movable_object(boss_copy[i]);
        }
        for (int i = 0; i < MAX_NUM_BULLETS; ++i)
        {
            draw_movable_object(bullets[selected_bullet_id][i]);
        }
        al_draw_rectangle(550, 50, 50, 120, al_map_rgb(150, 150, 150), 5);
        al_draw_filled_rectangle(60 + ((540 - 60) * plane.hp / 100.0), 60, 60, 110, al_map_rgb(255, 0, 0));
        al_draw_filled_rectangle(540, 60, 60 + ((540 - 60) * plane.hp / 100.0), 110, al_map_rgb(100, 0, 0));
        for (int i = 0; i < plane.lives; ++i)
        {
            al_draw_bitmap(img_heart, 560 + (40 * i), 60, 0);
        }
        al_draw_textf(font_pirulen_32, al_map_rgb(255, 255, 255), 310, 65, ALLEGRO_ALIGN_CENTRE, "%d%c", plane.hp, '\%');
        al_draw_rectangle(1550, 50, 1350, 125, al_map_rgb(255, 255, 255), 10);
        int duration = (int)(end_time - begin_time);
        al_draw_textf
        (
            font_pirulen_48, al_map_rgb(255, 255, 255), 1450, 60, ALLEGRO_ALIGN_CENTRE,
            "%02d%c%02d", (duration / 60), ':', (duration % 60)
        );
    }
    // [HACKATHON 3-9]
    // TODO: If active_scene is SCENE_SETTINGS.
    else if (active_scene == SCENE_SETTINGS)
    {
        al_clear_to_color(al_map_rgb(0, 0, 0));
        al_draw_rectangle(1500, 80, 100, 680, al_map_rgb(255, 255, 255), 10 );
        al_draw_text ( font_pirulen_48, al_map_rgb(255, 255, 255), 282, 203, ALLEGRO_ALIGN_CENTER, "PLANES" );
       
        //selected plane id 0
        if ( pnt_in_rect(mouse_x, mouse_y, 645, 170, 150, 150) || (selected_plane_id == 0) )
        {
            al_draw_rectangle( 795, 170, 645, 320, al_map_rgb(255, 213, 0), 10);
        }
        al_draw_bitmap( img_plane[0], 585, 95, 0 );
        
        //selected plane id 1
        if ( pnt_in_rect(mouse_x, mouse_y, 895, 170, 150, 150) || (selected_plane_id == 1) )
        {
            al_draw_rectangle ( 1045, 170, 895, 320, al_map_rgb(255, 213, 0), 10 );
        }
        al_draw_bitmap ( img_plane[1], 885, 155, 0 );
        
        //selected plane id 2
        if( pnt_in_rect(mouse_x, mouse_y, 1165, 170, 150, 150) || (selected_plane_id == 2) )
        {
            al_draw_rectangle ( 1315, 170, 1165, 320, al_map_rgb(255, 213, 0), 10 );
        }
        al_draw_bitmap ( img_plane[2], 1155, 155, 0 );
        
        //
        al_draw_line ( 460, 80, 460, 680, al_map_rgb(255, 255, 255), 10 );
        al_draw_text ( font_pirulen_48, al_map_rgb(255, 255, 255), 282, 428, ALLEGRO_ALIGN_CENTER, "BULLETS" );
        al_draw_bitmap ( img_bullet[0], 713, 443, 0 );
        
        if ( pnt_in_rect(mouse_x, mouse_y, 689, 425, 70, 70) || (selected_bullet_id == 0))
            al_draw_rectangle ( 759, 425, 689, 495, al_map_rgb(255, 213, 0), 10 );
            al_draw_bitmap ( img_bullet[1], 965, 443, 0 );
        
        if (pnt_in_rect(mouse_x, mouse_y, 940, 425, 70, 70) || (selected_bullet_id == 1) )
            al_draw_rectangle ( 1010, 425, 940, 495, al_map_rgb(255, 213, 0), 10 );
            al_draw_bitmap ( img_bullet[2], 1230, 443, 0 );
        if ( pnt_in_rect(mouse_x, mouse_y, 1200, 425, 70, 70) || (selected_bullet_id == 2) )
        {
            al_draw_rectangle ( 1280, 425, 1200, 495, al_map_rgb(255, 231, 0), 10 );
        }
        
        al_draw_line ( 1500, 380, 100, 380, al_map_rgb(255, 255, 255), 10 );
        al_draw_text ( font_pirulen_48, al_map_rgb(255, 255, 255), 282, 575, ALLEGRO_ALIGN_CENTER, "LEVELS" );
        
        al_draw_line ( 100, 530, 1500, 530, al_map_rgb(255, 255, 255), 10 );
        
        if ( pnt_in_rect(mouse_x, mouse_y, 650, 565, 250, 80) || (selected_level_id == 0) )
        {
            al_draw_rectangle ( 900, 565, 650, 645, al_map_rgb(255, 213, 0), 10 );
        }
        al_draw_text ( font_pirulen_48, al_map_rgb(255, 128, 0), 780, 578, ALLEGRO_ALIGN_CENTER, "EASY" );
        
        if ( pnt_in_rect(mouse_x, mouse_y, 1020, 565, 250, 80) || (selected_level_id == 1) )
        {
            al_draw_rectangle (  1270, 565, 1020, 645, al_map_rgb(255, 213, 0), 10 );
        }
        al_draw_text ( font_pirulen_48, al_map_rgb(178, 102, 255), 1148, 578, ALLEGRO_ALIGN_CENTER, "HARD" );
        
        al_draw_rectangle ( 750, 745, 500, 845, al_map_rgb(0, 127, 0), 10 );
        al_draw_text ( font_pirulen_48, al_map_rgb(0, 127, 0), 630, 765, ALLEGRO_ALIGN_CENTER, "OKAY" );
        
        al_draw_rectangle ( 1100, 745, 850, 845, al_map_rgb(127, 0, 0), 10 );
        al_draw_text ( font_pirulen_48, al_map_rgb(127, 0, 0), 978, 765, ALLEGRO_ALIGN_CENTER, "BACK" );
    }
    else if (active_scene == SCENE_WIN)
    {
        al_draw_bitmap(img_scene_win_background, 0, 0, 0);
        al_draw_rectangle(650,650, 950, 550, al_map_rgb(255,255,255), 10);
        int duration = (int)(end_time - begin_time);
        al_draw_textf ( font_pirulen_64, al_map_rgb(255, 255, 255), 800, 560, ALLEGRO_ALIGN_CENTRE,
            "%02d%c%02d", (duration / 60), ':', (duration % 60) );
        if (pnt_in_rect(mouse_x, mouse_y, 650, 550, 300, 100))
        {
            al_draw_rectangle ( 650, 650, 950, 550, al_map_rgb(255, 213, 0), 10 );
            al_draw_filled_rectangle(660, 640, 940, 560, al_map_rgb(0, 0, 0));
            al_draw_textf ( font_pirulen_64, al_map_rgb(255, 213, 0), 800, 560, ALLEGRO_ALIGN_CENTRE, "MENU" );
        }
    }
    
    else if (active_scene == SCENE_LOSE)
    {
        al_draw_bitmap(img_scene_lose_background, 0, 0, 0);
        al_draw_rectangle(630,695, 950, 595, al_map_rgb(255,255,255), 10);
        al_draw_textf ( font_pirulen_48, al_map_rgb(255, 255, 255), 790, 615, ALLEGRO_ALIGN_CENTRE, "MENU" );
        if (pnt_in_rect(mouse_x, mouse_y, 630, 595, 320, 100))
        {
            al_draw_rectangle ( 630, 695, 950, 595, al_map_rgb(255, 213, 0), 10 );
            //al_draw_filled_rectangle(660, 640, 940, 560, al_map_rgb(0, 0, 0));
            al_draw_textf ( font_pirulen_48, al_map_rgb(255, 213, 0), 790, 615, ALLEGRO_ALIGN_CENTRE, "MENU" );
        }

    }
    al_flip_display();
    return;
}

void game_destroy (void)
{
    al_destroy_sample(audio_scene_menu_background);
    al_destroy_sample(audio_scene_start_background);

    al_destroy_font(font_pirulen_64);
    al_destroy_font(font_pirulen_48);
    al_destroy_font(font_pirulen_32);

    al_destroy_bitmap(img_scene_menu_background);
    al_destroy_bitmap(img_scene_start_background);
    // [HACKATHON 3-6]
    // TODO: Destroy the 2 settings images.
    al_destroy_bitmap(img_settings_button_off);
    al_destroy_bitmap(img_settings_button_on);

    al_destroy_bitmap(img_enemy);
    // [HACKATHON 2-10]
    // TODO: Destroy your bullet image.
    for (int i = 0; i < 3; ++i)
    {
        al_destroy_bitmap(img_plane[i]);
        al_destroy_bitmap(img_bullet[i]);
    }

    al_destroy_timer(game_update_timer);
    al_destroy_event_queue(game_event_queue);
    al_destroy_display(game_display);

    free(mouse_state);

    return;
}

void game_change_scene (int next_scene)
{
    game_log("Change scene from %d to %d", active_scene, next_scene);
    // TODO: Destroy resources initialized when creating scene.
    if (active_scene == SCENE_MENU)
    {
        al_stop_sample(&audio_scene_menu_background_id);
        game_log("stop audio (bgm)");
        memset((void*)(&plane), 0, sizeof(MovableObject));
        for (int i = 0; i < 3; ++i)
        {
            for (int j = 0; j < MAX_NUM_BULLETS; ++j)
            {
                memset((void*)(&bullets[i][j]), 0, sizeof(MovableObject));
            }
        }
        for (int i = 0; i < MAX_NUM_ENEMIES; ++i)
        {
            memset((void*)(&enemies[i]), 0, sizeof(MovableObject));
        }
    }
    
    else if (active_scene == SCENE_START)
    {
        al_stop_sample(&audio_scene_start_background_id);
        game_log("stop audio (bgm)");
    }
    active_scene = next_scene;
    // TODO: Allocate resources before entering scene.
    if (active_scene == SCENE_MENU)
    {
        if (!al_play_sample(audio_scene_menu_background, 1, 0.0, 1.0, ALLEGRO_PLAYMODE_LOOP, &audio_scene_menu_background_id))
        {
            game_abort("failed to play audio (bgm)");
        }
    }
    else if (active_scene == SCENE_START)
    {
        plane.img = img_plane[selected_plane_id];
        plane.x = 800;
        plane.y = 450;
        plane.w = al_get_bitmap_width(plane.img);
        plane.h = al_get_bitmap_height(plane.img);
        plane.hp = 100;
        plane.lives = 3;
        if (selected_level_id == 0)
        {
            num_enemies = 8;
        }
        else
        {
            num_enemies = 16;
        }
        for (int i = 0; i < num_enemies; ++i)
        {
            enemies[i].img = img_enemy;
            enemies[i].x = ((enemies[i].w / 2) + (rand() % SCREEN_W));
            enemies[i].y = ((enemies[i].h / 2) + (rand() % SCREEN_H));
            if ( collision ( enemies[i].x, enemies[i].y, enemies[i].w, enemies[i].h, plane.x, plane.y, plane.w, plane.h))
            {
                enemies[i].x = ((enemies[i].w / 2) + (rand() % ((SCREEN_W - ENEMY_W) / 2)));
                enemies[i].y = ((enemies[i].h / 2) + (rand() % ((SCREEN_H - ENEMY_H) / 2)));
            }
            enemies[i].dir = rand() % 8;
            enemies[i].vx = vx_dir[enemies[i].dir] * (1 + rand() % 5);
            enemies[i].vy = vy_dir[enemies[i].dir] * (1 + rand() % 5);
            enemies[i].lives = 3;
            enemies[i].w = al_get_bitmap_width(img_enemy);
            enemies[i].h = al_get_bitmap_height(img_enemy);
        }

        // [HACKATHON 2-6]
        // TODO: Initialize bullets.
        for (int i = 0; i < MAX_NUM_BULLETS; ++i)
        {
          bullets[selected_bullet_id][i].w = al_get_bitmap_width(img_bullet[selected_bullet_id]);
          bullets[selected_bullet_id][i].h = al_get_bitmap_height(img_bullet[selected_bullet_id]);
          bullets[selected_bullet_id][i].img = img_bullet[selected_bullet_id];
          bullets[selected_bullet_id][i].hidden = true;
        }
        if (!al_play_sample(audio_scene_start_background, 1, 0.0, 1.0, ALLEGRO_PLAYMODE_LOOP, &audio_scene_start_background_id))
        {
            game_abort("failed to play audio (bgm)");
        }
    }
    else if (active_scene == SCENE_BOSS)
    {
        boss.img = img_boss;
        boss.x = ((boss.w / 2) + (rand() % (SCREEN_W / 2)));
        boss.y = ((boss.h / 2) + (rand() % (SCREEN_H / 2)));
        boss.w = al_get_bitmap_width(boss.img);
        boss.h = al_get_bitmap_height(boss.img);
        boss.lives = 10;
        boss.dir = rand() % 8;
        boss.last_shoot_timestamp = al_get_time();
        for (int i = 0; i < MAX_NUM_BOSSES; ++i)
        {
            boss_copy[i].img = img_boss_copy;
            boss_copy[i].dir = 0;
            boss_copy[i].vx = vx_dir[i];
            boss_copy[i].vy = vy_dir[i];
            boss_copy[i].w = al_get_bitmap_width(img_boss_copy);
            boss_copy[i].h = al_get_bitmap_height(img_boss_copy);
        }
    }
    return;
}

void on_key_down (int keycode)
{
    if (active_scene == SCENE_MENU)
    {
        if (keycode == ALLEGRO_KEY_ENTER)
        {
            begin_time = al_get_time();
            game_change_scene(SCENE_START);
        }
    }
    // [HACKATHON 3-10]
    // TODO: If active_scene is SCENE_SETTINGS and Backspace is pressed,
    // return to SCENE_MENU.
    // we don't need this anymore !!!
    return;
}

void on_mouse_down (int btn, int x, int y)
{
    // [HACKATHON 3-8]
    // TODO: When settings clicked, switch to settings scene.
    // Uncomment and fill in the code below.
    if (active_scene == SCENE_MENU)
    {
        if (btn == 1)
        {
            if (pnt_in_rect(x, y, 1470, 30, 100, 100))
            {
                game_change_scene(SCENE_SETTINGS);
            }
        }
    }
    else if (active_scene == SCENE_SETTINGS)
    {
        if (btn == 1)
        {
            if (pnt_in_rect(mouse_x, mouse_y, 500, 745, 250, 100))
            {
                game_change_scene(SCENE_MENU);
            }
            else if (pnt_in_rect(mouse_x, mouse_y, 850, 745, 250, 100))
            {
                selected_plane_id = 0;
                selected_bullet_id = 0;
                selected_level_id = 0;
                game_change_scene(SCENE_MENU);
            }
            if (pnt_in_rect(mouse_x, mouse_y, 645, 170, 150, 150))
            {
                selected_plane_id = 0;
            }
            else if (pnt_in_rect(mouse_x, mouse_y, 895, 170, 150, 150))
            {
                selected_plane_id = 1;
            }
            else if (pnt_in_rect(mouse_x, mouse_y, 1165, 170, 150, 150))
            {
                selected_plane_id = 2;
            }
            else if (pnt_in_rect(mouse_x, mouse_y, 689, 425, 70, 70))
            {
                selected_bullet_id = 0;
            }
            else if (pnt_in_rect(mouse_x, mouse_y, 940, 425, 70, 70))
            {
                selected_bullet_id = 1;
            }
            else if (pnt_in_rect(mouse_x, mouse_y, 1200, 425, 70, 70))
            {
                selected_bullet_id = 2;
            }
            else if (pnt_in_rect(mouse_x, mouse_y, 650, 565, 250, 80))
            {
                selected_level_id = 0;
            }
            else if (pnt_in_rect(mouse_x, mouse_y, 1020, 565, 250, 80))
            {
                selected_level_id = 1;
            }
        }
    }
    else if (active_scene == SCENE_WIN)
    {
        if (btn == 1)
        {
            if (pnt_in_rect(mouse_x, mouse_y, 650, 550, 300, 100))
            {
                game_change_scene(SCENE_MENU);
            }
        }
    }
    else if (active_scene == SCENE_LOSE)
    {
        if (btn == 1)
        {
            if (pnt_in_rect(mouse_x, mouse_y, 630, 595, 320, 100))
            {
                game_change_scene(SCENE_MENU);
            }
        }
    }
    return;
}

void draw_movable_object (MovableObject obj)
{
    const float rad_45 = 0.785398f;
    if (!obj.hidden)
    {
        al_draw_rotated_bitmap ( obj.img, (obj.w / 2), (obj.h / 2), obj.x, obj.y, (rad_45 * obj.dir), 0 );
    }
    return;
}

ALLEGRO_BITMAP *load_bitmap_resized (const char *filename, int w, int h)
{
    ALLEGRO_BITMAP* loaded_bmp = al_load_bitmap(filename);
    if (!loaded_bmp)
        game_abort("failed to load image: %s", filename);
    ALLEGRO_BITMAP *resized_bmp = al_create_bitmap(w, h);
    ALLEGRO_BITMAP *prev_target = al_get_target_bitmap();

    if (!resized_bmp)
        game_abort("failed to create bitmap when creating resized image: %s", filename);
    al_set_target_bitmap(resized_bmp);
    al_draw_scaled_bitmap ( loaded_bmp, 0, 0, al_get_bitmap_width(loaded_bmp),
        al_get_bitmap_height(loaded_bmp), 0, 0, w, h, 0 );
    al_set_target_bitmap(prev_target);
    al_destroy_bitmap(loaded_bmp);

    game_log("resized image: %s", filename);

    return resized_bmp;
}

// [HACKATHON 3-3]
// TODO: Define bool pnt_in_rect(int px, int py, int x, int y, int w, int h)
bool pnt_in_rect (int px, int py, int x, int y, int w, int h)
{
    return( (px <= (x + w)) && (py <= (y + h)) && (px >= x) && (py >= y) );
}

bool collision (float x1, float y1, float w1, float h1, float x2, float y2, float w2, float h2)
{
    return ( (x1 - (w1 / 3) <= x2 + (w2 / 3)) && (x1 + (w1 / 3) >= x2 - (w2 / 3)) &&
        (y1 - (h1 / 3) <= y2 + (h2 / 3)) && (y1 + (h1 / 3) >= y2 - (h2 / 3)) );
}

bool is_hit_boundary (float x, float y, float w, float h)
{
    return ( (x - (w / 2) < 0) || (x + (w / 2) > SCREEN_W) || (y - (h / 2) < 0) || (y + (h / 2) > SCREEN_H) );
}

// +=================================================================+
// | Code below is for debugging purpose, it's fine to remove it.    |
// | Deleting the code below and removing all calls to the functions |
// | doesn't affect the game.                                        |
// +=================================================================+

void game_abort(const char* format, ...) {
    va_list arg;
    va_start(arg, format);
    game_vlog(format, arg);
    va_end(arg);
    fprintf(stderr, "error occured, exiting after 2 secs");
    // Wait 2 secs before exiting.
    al_rest(2);
    // Force exit program.
    exit(1);
}

void game_log(const char* format, ...) {
#ifdef LOG_ENABLED
    va_list arg;
    va_start(arg, format);
    game_vlog(format, arg);
    va_end(arg);
#endif
}

void game_vlog(const char* format, va_list arg) {
#ifdef LOG_ENABLED
    static bool clear_file = true;
    // Write log to file for later debugging.
    FILE* pFile = fopen("log.txt", clear_file ? "w" : "a");
    if (pFile) {
        va_list arg2;
        va_copy(arg2, arg);
        vfprintf(pFile, format, arg2);
        va_end(arg2);
        fprintf(pFile, "\n");
        fclose(pFile);
    }
    vprintf(format, arg);
    printf("\n");
    clear_file = false;
#endif
}
