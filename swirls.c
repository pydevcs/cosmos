#include <furi.h>
#include <gui/gui.h>
#include <math.h>

// Define a simple structure for our swirl "particles"
typedef struct {
    float x, y;
    float radius;
    uint8_t lifetime;
} Swirl;

#define MAX_SWIRLS 5

// The render callback where the magic happens
static void render_callback(Canvas* canvas, void* ctx) {
    Swirl* swirls = (Swirl*)ctx;

    canvas_set_color(canvas, ColorBlack);
    
    for(int i = 0; i < MAX_SWIRLS; i++) {
        if(swirls[i].lifetime > 0) {
            // Draw the "swirl" as an ellipse
            // We use radius for width and radius/2 for height to get that perspective look
            canvas_draw_ellipse(
                canvas, 
                swirls[i].x, 
                swirls[i].y, 
                swirls[i].radius, 
                swirls[i].radius / 2
            );

            // Logic: Expand and fade
            swirls[i].radius += 1.5f;
            swirls[i].lifetime--;

            // Adding "trippy" dithering or secondary rings
            if(swirls[i].lifetime % 4 == 0) {
                canvas_draw_circle(canvas, swirls[i].x, swirls[i].y, swirls[i].radius - 2);
            }
        } else {
            // Reset swirl to fin position (example coordinates)
            swirls[i].x = 40; 
            swirls[i].y = 32;
            swirls[i].radius = 1;
            swirls[i].lifetime = 20 + (rand() % 10);
        }
    }
}