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

#define STAR_COUNT   36
#define PLANET_COUNT 3
#define COMET_COUNT  2
#define ENEMY_COUNT  3
#define PULSE_MAX    16

#define WARP_STAR_MULT    6.0f
#define WARP_PLANET_MULT  3.0f
#define WARP_COMET_MULT   4.0f

typedef struct { float x,y,speed; } Star;
typedef struct { float angle,radius,speed; uint8_t size; } Planet;
typedef struct { float x,y,vx,vy; } Comet;
typedef struct { float x,y; } Ship;
typedef struct { float x,y; bool alive; } Enemy;
typedef struct { int r; bool active; } Pulse;

typedef struct {
    Star stars[STAR_COUNT];
    Planet planets[PLANET_COUNT];
    Comet comets[COMET_COUNT];
    Enemy enemies[ENEMY_COUNT];
    Ship ship;
    Pulse pulse;
    int cam_x, cam_y;
    uint32_t tick;
    bool warp;
    bool exit;
} Cosmos;

/* ---------- Init ---------- */
static void cosmos_init(Cosmos* c){
    for(int i=0;i<STAR_COUNT;i++)
        c->stars[i]=(Star){rand()%W,rand()%H,0.3f+(i%3)*0.4f};

    for(int i=0;i<PLANET_COUNT;i++)
        c->planets[i]=(Planet){rand()%360,12+i*14,0.01f+i*0.01f,4+i};

    for(int i=0;i<COMET_COUNT;i++)
        c->comets[i]=(Comet){W+rand()%40,rand()%H,-2.0f,0.6f};

    for(int i=0;i<ENEMY_COUNT;i++)
        c->enemies[i]=(Enemy){W+rand()%40,rand()%H,true};

    c->ship=(Ship){20,H/2};
    c->pulse=(Pulse){0,false};
    c->warp=false;
    c->exit=false;
    c->tick=0;
}

/* ---------- Dolphin UFO ---------- */
static void draw_dolphin(Canvas* cv,int x,int y){
    canvas_draw_line(cv,x-9,y,x+9,y);
    canvas_draw_line(cv,x-6,y-2,x+6,y-2);
    canvas_draw_line(cv,x-4,y-4,x+4,y-4);
    canvas_draw_line(cv,x,y-4,x+1,y-7);        // dorsal fin
    canvas_draw_line(cv,x+6,y-1,x+9,y);        // nose
    canvas_draw_line(cv,x-9,y,x-11,y-1);       // tail
    canvas_draw_line(cv,x-9,y,x-11,y+1);
    canvas_draw_dot(cv,x+3,y-2);               // eye
}

/* ---------- Alien Enemy ---------- */
static void draw_enemy(Canvas* cv,int x,int y){
    canvas_draw_circle(cv,x,y,3);
    canvas_draw_line(cv,x-4,y,x+4,y);
    canvas_draw_dot(cv,x,y);
}

/* ---------- Alien HUD Glyph ---------- */
static void draw_glyph(Canvas* cv,int x,int y,uint32_t t){
    if((t>>3)&1){
        canvas_draw_line(cv,x,y,x+4,y+4);
        canvas_draw_line(cv,x+4,y,x,y+4);
    } else {
        canvas_draw_circle(cv,x+2,y+2,2);
    }
}

/* ---------- Update ---------- */
static void cosmos_update(Cosmos* c){
    c->tick++;

    c->cam_x = sinf(c->tick*0.01f)*4;
    c->cam_y = cosf(c->tick*0.008f)*3;

    /* Stars */
    for(int i=0;i<STAR_COUNT;i++){
        float sp=c->stars[i].speed*(c->warp?WARP_STAR_MULT:1);
        c->stars[i].x-=sp;
        if(c->stars[i].x<0){ c->stars[i].x=W; c->stars[i].y=rand()%H; }
    }

    /* Planets + gravity wells */
    for(int i=0;i<PLANET_COUNT;i++){
        c->planets[i].angle += c->planets[i].speed*(c->warp?WARP_PLANET_MULT:1);

        float px=W/2+cosf(c->planets[i].angle)*c->planets[i].radius;
        float py=H/2+sinf(c->planets[i].angle)*c->planets[i].radius;
        float dx=px-c->ship.x, dy=py-c->ship.y;
        float d=sqrtf(dx*dx+dy*dy);
        if(d<18){
            c->ship.x+=dx/d*0.4f;
            c->ship.y+=dy/d*0.4f;
        }
    }

    /* Comets */
    for(int i=0;i<COMET_COUNT;i++){
        float m=c->warp?WARP_COMET_MULT:1;
        c->comets[i].x+=c->comets[i].vx*m;
        c->comets[i].y+=c->comets[i].vy*m;
        if(c->comets[i].x<-20||c->comets[i].y>H+20)
            c->comets[i]=(Comet){W+rand()%40,rand()%H,-2.0f,0.6f};
    }

    /* Enemies */
    for(int i=0;i<ENEMY_COUNT;i++){
        if(!c->enemies[i].alive) continue;
        c->enemies[i].x-=1.2f;
        if(c->enemies[i].x<0){
            c->enemies[i].x=W+rand()%20;
            c->enemies[i].y=rand()%H;
        }
    }

    /* Sonar pulse */
    if(c->pulse.active){
        c->pulse.r++;
        for(int i=0;i<ENEMY_COUNT;i++){
            if(!c->enemies[i].alive) continue;
            float dx=c->enemies[i].x-c->ship.x;
            float dy=c->enemies[i].y-c->ship.y;
            if(sqrtf(dx*dx+dy*dy)<c->pulse.r)
                c->enemies[i].alive=false;
        }
        if(c->pulse.r>PULSE_MAX)
            c->pulse.active=false;
    }
}

/* ---------- Draw ---------- */
static void cosmos_draw(Canvas* cv,Cosmos* c){
    canvas_clear(cv);

    /* Stars */
    for(int i=0;i<STAR_COUNT;i++)
        canvas_draw_dot(cv,c->stars[i].x+c->cam_x,c->stars[i].y+c->cam_y);

    /* Warp streaks */
    if(c->warp){
        for(int i=0;i<STAR_COUNT;i++)
            canvas_draw_dot(cv,c->stars[i].x-2,c->stars[i].y);
    }

    /* Planets */
    for(int i=0;i<PLANET_COUNT;i++){
        int x=W/2+cosf(c->planets[i].angle)*c->planets[i].radius;
        int y=H/2+sinf(c->planets[i].angle)*c->planets[i].radius;
        canvas_draw_circle(cv,x+c->cam_x,y+c->cam_y,c->planets[i].size);
    }

    /* Comets */
    for(int i=0;i<COMET_COUNT;i++)
        for(int t=0;t<(c->warp?7:4);t++)
            canvas_draw_dot(cv,
                c->comets[i].x-t*c->comets[i].vx+c->cam_x,
                c->comets[i].y-t*c->comets[i].vy+c->cam_y);

    /* Enemies */
    for(int i=0;i<ENEMY_COUNT;i++)
        if(c->enemies[i].alive)
            draw_enemy(cv,c->enemies[i].x+c->cam_x,c->enemies[i].y+c->cam_y);

    /* Sonar pulse */
    if(c->pulse.active)
        canvas_draw_circle(cv,c->ship.x,c->ship.y,c->pulse.r);

    /* HUD glyphs */
    for(int i=0;i<5;i++)
        draw_glyph(cv,5+i*6,2,c->tick+i);

    /* Player */
    draw_dolphin(cv,c->ship.x+c->cam_x,c->ship.y+c->cam_y);
}

/* ---------- Input ---------- */
static void input_cb(InputEvent* e,void* ctx){
    Cosmos* c=ctx;

    if(e->type==InputTypePress){
        switch(e->key){
            case InputKeyUp:    c->ship.y-=2; break;
            case InputKeyDown:  c->ship.y+=2; break;
            case InputKeyLeft:  c->ship.x-=2; break;
            case InputKeyRight: c->ship.x+=2; break;
            case InputKeyOk:
                c->warp=true;
                c->pulse=(Pulse){0,true};
                NotificationApp* n=furi_record_open(RECORD_NOTIFICATION);
                notification_message(n,&sequence_single_vibro);
                furi_record_close(RECORD_NOTIFICATION);
                break;
            case InputKeyBack: c->exit=true; break;
            default: break;
        }
    }

    if(e->type==InputTypeRelease && e->key==InputKeyOk)
        c->warp=false;
}

/* ---------- App ---------- */
int32_t cosmos_app(void* p){
    UNUSED(p);
    Cosmos c; cosmos_init(&c);

    ViewPort* vp=view_port_alloc();
    view_port_draw_callback_set(vp,(ViewPortDrawCallback)cosmos_draw,&c);
    view_port_input_callback_set(vp,input_cb,&c);

    Gui* gui=furi_record_open(RECORD_GUI);
    gui_add_view_port(gui,vp,GuiLayerFullscreen);

    while(!c.exit){
        cosmos_update(&c);
        view_port_update(vp);
        furi_delay_ms(50);
    }

    gui_remove_view_port(gui,vp);
    view_port_free(vp);
    furi_record_close(RECORD_GUI);
    return 0;
}
