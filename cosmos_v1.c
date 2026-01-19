
#include <furi.h>
#include <gui/view_port.h>
#include <gui/gui.h>
#include <input/input.h>
#include <math.h>
#include <stdlib.h>
#include <notification/notification_messages.h>

#define W 128
#define H 64

#define STAR_COUNT 30
#define PLANET_COUNT 3
#define COMET_COUNT 2

typedef struct { float x, y, speed; } Star;
typedef struct { float angle, radius, speed; uint8_t size; } Planet;
typedef struct { float x, y, vx, vy; } Comet;

typedef struct {
    float x, y;
} Ship;

typedef struct {
    Star stars[STAR_COUNT];
    Planet planets[PLANET_COUNT];
    Comet comets[COMET_COUNT];
    Ship ship;
    int cam_x;
    int cam_y;
    uint32_t tick;
} Cosmos;

static void cosmos_init(Cosmos* c) {
    for(int i=0;i<STAR_COUNT;i++) {
        c->stars[i]=(Star){rand()%W, rand()%H, 0.3f+(i%3)*0.4f};
    }
    for(int i=0;i<PLANET_COUNT;i++) {
        c->planets[i]=(Planet){rand()%360, 10+i*14, 0.01f+i*0.01f, 4+i};
    }
    for(int i=0;i<COMET_COUNT;i++) {
        c->comets[i]=(Comet){W+rand()%40, rand()%20, -2.0f, 0.7f};
    }
    c->ship.x = 20;
    c->ship.y = H/2;
}

static void draw_dithered_circle(Canvas* canvas, int cx, int cy, int r) {
    for(int y=-r;y<=r;y++) for(int x=-r;x<=r;x++)
        if(x*x+y*y<=r*r && ((x+y)&1))
            canvas_draw_dot(canvas, cx+x, cy+y);
}

static void cosmos_update(Cosmos* c) {
    c->tick++;

    for(int i=0;i<STAR_COUNT;i++) {
        c->stars[i].x -= c->stars[i].speed;
        if(c->stars[i].x<0){c->stars[i].x=W;c->stars[i].y=rand()%H;}
    }

    for(int i=0;i<PLANET_COUNT;i++)
        c->planets[i].angle += c->planets[i].speed;

    for(int i=0;i<COMET_COUNT;i++) {
        c->comets[i].x += c->comets[i].vx;
        c->comets[i].y += c->comets[i].vy;
        if(fabsf(c->comets[i].x-c->ship.x)<6 && fabsf(c->comets[i].y-c->ship.y)<6) {
            NotificationApp* n = furi_record_open(RECORD_NOTIFICATION);
            notification_message(n, &sequence_single_vibro);
            furi_record_close(RECORD_NOTIFICATION);
        }
        if(c->comets[i].x<-20 || c->comets[i].y>H+20)
            c->comets[i]=(Comet){W+rand()%40, rand()%20, -2.0f, 0.7f};
    }

    // demo-scene camera drift (sinusoidal)
    c->cam_x = sinf(c->tick * 0.01f) * 4;
    c->cam_y = cosf(c->tick * 0.008f) * 3;
}

static void cosmos_draw(Canvas* canvas, Cosmos* c) {
    canvas_clear(canvas);
    canvas_set_color(canvas, ColorBlack);

    // Nebula (procedural noise dots)
    for(int i=0;i<40;i++)
        if(rand()%6==0)
            canvas_draw_dot(canvas, rand()%W, rand()%H);

    for(int i=0;i<STAR_COUNT;i++)
        canvas_draw_dot(canvas, c->stars[i].x+c->cam_x, c->stars[i].y+c->cam_y);

    int cx=W/2+c->cam_x, cy=H/2+c->cam_y;
    for(int i=0;i<PLANET_COUNT;i++) {
        int x=cx+cosf(c->planets[i].angle)*c->planets[i].radius;
        int y=cy+sinf(c->planets[i].angle)*c->planets[i].radius;
        draw_dithered_circle(canvas,x,y,c->planets[i].size);
    }

    for(int i=0;i<COMET_COUNT;i++)
        for(int t=0;t<4;t++)
            canvas_draw_dot(canvas,
                c->comets[i].x-t*c->comets[i].vx+c->cam_x,
                c->comets[i].y-t*c->comets[i].vy+c->cam_y);

    // Player ship
    int sx=c->ship.x+c->cam_x, sy=c->ship.y+c->cam_y;
    canvas_draw_line(canvas,sx-4,sy,sx+4,sy);
    canvas_draw_line(canvas,sx-4,sy,sx-8,sy-2);
    canvas_draw_line(canvas,sx-4,sy,sx-8,sy+2);
}

static void input_cb(InputEvent* e, void* ctx) {
    Cosmos* c=ctx;
    if(e->type==InputTypePress||e->type==InputTypeRepeat) {
        if(e->key==InputKeyUp) c->ship.y-=2;
        if(e->key==InputKeyDown) c->ship.y+=2;
        if(e->key==InputKeyLeft) c->ship.x-=2;
        if(e->key==InputKeyRight) c->ship.x+=2;
    }
}

static void draw_cb(Canvas* canvas, void* ctx) {
    cosmos_draw(canvas, ctx);
}

int32_t cosmos_app(void* p) {
    UNUSED(p);

    Cosmos cosmos = {0};
    cosmos_init(&cosmos);

    ViewPort* vp = view_port_alloc();
    view_port_draw_callback_set(vp, draw_cb, &cosmos);
    view_port_input_callback_set(vp, input_cb, &cosmos);

    Gui* gui = furi_record_open(RECORD_GUI);
    gui_add_view_port(gui, vp, GuiLayerFullscreen);

    while(true) {
        cosmos_update(&cosmos);
        view_port_update(vp);
        furi_delay_ms(50);
    }

    gui_remove_view_port(gui, vp);
    view_port_free(vp);
    furi_record_close(RECORD_GUI);

    return 0;
}
