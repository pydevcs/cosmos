#include <furi.h>
#include <gui/gui.h>
#include <gui/view_port.h>
#include <gui/canvas.h>
#include <input/input.h>
#include <math.h>
#include <stdlib.h>
#include <notification/notification_messages.h>

#define W 128
#define H 64
#define STAR_COUNT 30
#define PLANET_COUNT 3
#define COMET_COUNT 2

typedef enum { ModeInteractive, ModeWarp, ModeSleep } CosmosMode;
typedef enum { DitherNone, DitherChecker, DitherXOR } DitherMode;

typedef struct { float x, y, speed; } Star;
typedef struct { float angle, radius, speed; uint8_t size; } Planet;
typedef struct { float x, y, vx, vy; } Comet;
typedef struct { float x, y; } Ship;

typedef struct {
    Star stars[STAR_COUNT];
    Planet planets[PLANET_COUNT];
    Comet comets[COMET_COUNT];
    Ship ship;
    int cam_x, cam_y;
    uint32_t tick;
    CosmosMode mode;
    DitherMode dither;
    bool warp;
    uint32_t idle_ticks;
    bool exit;
} Cosmos;

// ---------------- Initialization ----------------
static void cosmos_init(Cosmos* c) {
    for(int i=0;i<STAR_COUNT;i++)
        c->stars[i]=(Star){rand()%W, rand()%H, 0.3f+(i%3)*0.4f};

    for(int i=0;i<PLANET_COUNT;i++)
        c->planets[i]=(Planet){rand()%360, 10+i*14, 0.01f+i*0.01f, 3+i};

    for(int i=0;i<COMET_COUNT;i++)
        c->comets[i]=(Comet){W+rand()%40, rand()%20, -2.0f, 0.7f};

    c->ship.x = 20;
    c->ship.y = H/2;
    c->cam_x = c->cam_y = 0;
    c->tick = 0;
    c->mode = ModeInteractive;
    c->dither = DitherChecker;
    c->warp = false;
    c->idle_ticks = 0;
    c->exit = false;
}

// ---------------- Drawing Utilities ----------------
static void draw_dithered_circle(Canvas* canvas,int cx,int cy,int r,DitherMode mode,uint32_t tick){
    for(int y=-r;y<=r;y++)
        for(int x=-r;x<=r;x++){
            if(x*x+y*y<=r*r){
                bool draw=false;
                switch(mode){
                    case DitherChecker: draw=((x+y)&1); break;
                    case DitherXOR: draw=((x^y^tick)&1); break;
                    default: draw=true;
                }
                if(draw) canvas_draw_dot(canvas,cx+x,cy+y);
            }
        }
}

static void draw_ufo(Canvas* canvas, int x, int y) {
    // UFO base using lines to approximate ellipse
    canvas_draw_line(canvas, x-8, y, x+8, y);       // bottom
    canvas_draw_line(canvas, x-6, y-2, x+6, y-2);   // mid
    canvas_draw_line(canvas, x-4, y-4, x+4, y-4);   // top

    // Dome
    canvas_draw_circle(canvas, x, y-5, 3);

    // Lights
    canvas_draw_dot(canvas, x-5, y+1);
    canvas_draw_dot(canvas, x, y+2);
    canvas_draw_dot(canvas, x+5, y+1);
}

// ---------------- Update ----------------
static void cosmos_update(Cosmos* c){
    c->tick++;
    c->idle_ticks++;

    // Camera drift
    c->cam_x = sinf(c->tick*0.01f)*4;
    c->cam_y = cosf(c->tick*0.008f)*3;

    // Stars
    for(int i=0;i<STAR_COUNT;i++){
        float speed = c->stars[i].speed;
        if(c->warp) speed *= 6.0f;  // warp multiplier
        c->stars[i].x -= speed;
        if(c->stars[i].x < 0){
            c->stars[i].x = W;
            c->stars[i].y = rand() % H;
        }
    }

    // Planets
    for(int i=0;i<PLANET_COUNT;i++)
        c->planets[i].angle += c->planets[i].speed;

    // Comets
    for(int i=0;i<COMET_COUNT;i++){
        c->comets[i].x += c->comets[i].vx;
        c->comets[i].y += c->comets[i].vy;

        // vibrate on pass
        if(fabsf(c->comets[i].x-c->ship.x)<6 && fabsf(c->comets[i].y-c->ship.y)<6){
            NotificationApp* n = furi_record_open(RECORD_NOTIFICATION);
            notification_message(n, &sequence_single_vibro);
            furi_record_close(RECORD_NOTIFICATION);
        }

        if(c->comets[i].x<-20 || c->comets[i].y>H+20)
            c->comets[i] = (Comet){W+rand()%40, rand()%20, -2.0f, 0.7f};
    }
}

// ---------------- Draw ----------------
static void cosmos_draw(Canvas* canvas, Cosmos* c){
    canvas_clear(canvas);
    canvas_set_color(canvas, ColorBlack);

    if(c->mode==ModeSleep){ furi_delay_ms(200); return; }

    // Nebula dots
    for(int i=0;i<40;i++)
        if(rand()%6==0)
            canvas_draw_dot(canvas, rand()%W, rand()%H);

    // Stars
    for(int i=0;i<STAR_COUNT;i++)
        canvas_draw_dot(canvas, c->stars[i].x + c->cam_x, c->stars[i].y + c->cam_y);

    // Warp flash streaks
    if(c->warp){
        for(int i=0;i<STAR_COUNT;i++){
            int sx = (int)(c->stars[i].x + c->cam_x);
            int sy = (int)(c->stars[i].y + c->cam_y);
            int len = 2 + (rand()%3); // streak length 2–4
            for(int s=1;s<=len;s++){
                int px = sx - (int)(c->stars[i].speed * s);
                if(px >=0 && px < W)
                    canvas_draw_dot(canvas, px, sy);
            }
        }
        // optional spark pixels
        for(int i=0;i<20;i++)
            canvas_draw_dot(canvas, rand()%W, rand()%H);
    }

    // Planets
    int cx=W/2 + c->cam_x, cy=H/2 + c->cam_y;
    for(int i=0;i<PLANET_COUNT;i++){
        int x = cx + cosf(c->planets[i].angle)*c->planets[i].radius;
        int y = cy + sinf(c->planets[i].angle)*c->planets[i].radius;
        draw_dithered_circle(canvas, x, y, c->planets[i].size, c->dither, c->tick);
    }

    // Comets
    for(int i=0;i<COMET_COUNT;i++)
        for(int t=0;t<4;t++)
            canvas_draw_dot(canvas,
                c->comets[i].x - t*c->comets[i].vx + c->cam_x,
                c->comets[i].y - t*c->comets[i].vy + c->cam_y);

    // Player UFO
    draw_ufo(canvas, c->ship.x + c->cam_x, c->ship.y + c->cam_y);
}

// ---------------- Input ----------------
static void input_cb(InputEvent* e, void* ctx){
    Cosmos* c = ctx;

    c->idle_ticks = 0;

    if(e->type == InputTypePress || e->type == InputTypeRepeat){
        switch(e->key){
            case InputKeyUp:    c->ship.y -= 2; break;
            case InputKeyDown:  c->ship.y += 2; break;
            case InputKeyLeft:  c->ship.x -= 2; break;
            case InputKeyRight: c->ship.x += 2; break;
            case InputKeyOk:    c->warp = true; c->mode = ModeWarp; break; // hold OK for warp
            case InputKeyBack:  c->exit = true; break;
            default: break;
        }
    }
    else if(e->type == InputTypeRelease){
        if(e->key == InputKeyOk){
            c->warp = false;
            c->mode = ModeInteractive;
        }
    }
}

// ---------------- Viewport ----------------
static void draw_cb(Canvas* canvas, void* ctx){ cosmos_draw(canvas, ctx); }

// ---------------- App Entry ----------------
int32_t cosmos_app(void* p){
    UNUSED(p);
    Cosmos cosmos = {0};
    cosmos_init(&cosmos);

    ViewPort* vp = view_port_alloc();
    view_port_draw_callback_set(vp, draw_cb, &cosmos);
    view_port_input_callback_set(vp, input_cb, &cosmos);

    Gui* gui = furi_record_open(RECORD_GUI);
    gui_add_view_port(gui, vp, GuiLayerFullscreen);

    while(!cosmos.exit){
        cosmos_update(&cosmos);
        view_port_update(vp);
        furi_delay_ms(50);
    }

    gui_remove_view_port(gui, vp);
    view_port_free(vp);
    furi_record_close(RECORD_GUI);

    return 0;
}
