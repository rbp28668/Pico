
#include <cmath>
#include <algorithm>
#include "../Displays/GFX.h"

// Display Constants for horizon.
const int WIDTH = 240;
const int HEIGHT = 320;
const int CENTER_X = 120;
const int CENTER_Y = 160;
const float PIXELS_PER_RADIAN = 200.0f; // Zoom level for pitch

const uint16_t COLOR_SKY_BLUE = 0x001F;
const uint16_t COLOR_GROUND_BROWN = 0x7BE0; // more olive!
const uint16_t COLOR_YELLOW = 0xFFE0;
const uint16_t COLOR_WHITE = 0xFFFF;

struct Point {
    float x, y;
};

void draw_artificial_horizon(float pitch, float roll, GFX& display) {
    // 1. Pre-calculate trig for performance
    float cos_r = cos(roll);
    float sin_r = sin(roll);
    float tan_r = tan(-roll); // keep horizon and pitch bars aligned.

    // 2. Clear background to sky (Blue)
    display.fillRect(0, 0, WIDTH, HEIGHT, COLOR_SKY_BLUE);
    display.fillScreen(COLOR_SKY_BLUE);
    // 3. Calculate Horizon Line
    // Vertical displacement due to pitch
    float dy = pitch * PIXELS_PER_RADIAN;

    // Calculate Y coordinates where the horizon hits the left and right edges
    // y = y_offset + (x_dist_from_center * tan(roll))
    float y_left = CENTER_Y + dy + (CENTER_X * tan_r);
    float y_right = CENTER_Y + dy - (CENTER_X * tan_r);

    // 4. Draw Ground (Brown)
    // We use two triangles and a rectangle to fill the area below the horizon line
    // This fills from the horizon line (y_left to y_right) to the bottom (320)
    display.fillTriangle(0, y_left, 240, y_right, 0, 320, COLOR_GROUND_BROWN);
    display.fillTriangle(240, y_right, 240, 320, 0, 320, COLOR_GROUND_BROWN);

    // 5. Draw Pitch Ladder (Every 5 degrees)
    for (int deg = -90; deg <= 90; deg += 5) {
        if (deg == 0) continue; // Skip horizon line for ladder

        float rad = deg * M_PI / 180.0f;
        float d_pitch = (rad - pitch) * PIXELS_PER_RADIAN;

        // Center of this pitch line
        float cx = CENTER_X + d_pitch * sin_r;
        float cy = CENTER_Y - d_pitch * cos_r;

        // Line width (shorter for 5 deg, longer for 10 deg)
        float w = (deg % 10 == 0) ? 40.0f : 20.0f;

        // Calculate endpoints of the rotated line segment
        float x1 = cx - (w / 2) * cos_r;
        float y1 = cy - (w / 2) * sin_r;
        float x2 = cx + (w / 2) * cos_r;
        float y2 = cy + (w / 2) * sin_r;

        display.drawLine(x1, y1, x2, y2, COLOR_WHITE);
    }

    // 6. Static Aircraft Reference (Center)
    // Fixed "W" shape or dot - does not move or rotate
    display.drawLine(CENTER_X - 20, CENTER_Y, CENTER_X - 5, CENTER_Y, COLOR_YELLOW);
    display.drawLine(CENTER_X + 5, CENTER_Y, CENTER_X + 20, CENTER_Y, COLOR_YELLOW);
    display.fillRect(CENTER_X - 2, CENTER_Y - 2, 4, 4, COLOR_YELLOW);
}
