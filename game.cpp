#include "game.h"

#define internal static
#define PAL16_VOID        0x000000FF
#define PAL16_ASH         0x9D9D9DFF
#define PAL16_BLIND       0xFFFFFFFF
#define PAL16_BLOODRED    0xBE2633FF
#define PAL16_PIGMEAT     0xE06F8BFF
#define PAL16_OLDPOOP     0x493C2BFF
#define PAL16_NEWPOOP     0xA46422FF
#define PAL16_BLAZE       0xEB8931FF
#define PAL16_ZORNSKIN    0xF7E26BFF
#define PAL16_SHADEGREEN  0x2F484EFF
#define PAL16_LEAFGREEN   0x44891AFF
#define PAL16_SLIMEGREEN  0xA3CE27FF
#define PAL16_NIGHTBLUE   0x1B2632FF
#define PAL16_SEABLUE     0x005784FF
#define PAL16_SKYBLUE     0x31A2F2FF
#define PAL16_CLOUDBLUE   0xB2DCEFFF

void SetPixel(GameFramebuffer &framebuffer, int x, int y, uint32 color)
{
    framebuffer.pixels[y * framebuffer.width + x] = color;
}

void DrawLine(GameFramebuffer &framebuffer, vec2 p1, vec2 p2)
{
    // Convert from normalized coordinates to pixel coordinates
    float x1 = p1.x * framebuffer.width;
    float x2 = p2.x * framebuffer.width;
    float y1 = p1.y * framebuffer.height;
    float y2 = p2.y * framebuffer.height;

    float dx = x2 - x1;
    float dy = y2 - y1;
    int n = max(abs(dx), abs(dy));
    for (int i = 0; i <= n; i++)
    {
        float t = (float)i / (float)n;
        int x = round(x1 + dx * t);
        int y = round(y1 + dy * t);
        x = max(min(x, framebuffer.width - 1), 0);
        y = max(min(y, framebuffer.height - 1), 0);
        SetPixel(framebuffer, x, y, 0xffffffff);
    }
}

void GameUpdateAndRender(GameFramebuffer &framebuffer,
                         GameMemory &memory,
                         GameInput &input)
{
    GameState *state = (GameState*)&memory;

    if (!memory.is_initialized)
    {
        memory.is_initialized = true;
    }

    GameRenderer *rnd = (GameRenderer*)&memory.renderer;
    rnd->SetColor(PAL16_CLOUDBLUE);
    rnd->Clear();
    rnd->SetColor(PAL16_BLOODRED);
    rnd->DrawLine(10, 10, 100, 50);

    // float t = memory.elapsed_time;
    // for (int y = 0; y < framebuffer.height; y++)
    // {
    //     for (int x = 0; x < framebuffer.width; x++)
    //     {
    //         uint8 r = uint8(y + 10.0f * t) % 64;
    //         uint8 g = uint8(x + 10.0f * t) % 64;
    //         uint8 b = 128;
    //         uint8 a = 255;
    //         uint32 color = (a << 24) | (r << 16) | (g << 8) | b;
    //         framebuffer.pixels[y * framebuffer.width + x] = color;
    //     }
    // }

    // internal vec2 line_end = Vec2(0.5f, 0.5f);
    // if (input.btn_left.is_down)
    //     line_end.x -= memory.frame_time;
    // else if (input.btn_right.is_down)
    //     line_end.x += memory.frame_time;
    // if (input.btn_up.is_down)
    //     line_end.y -= memory.frame_time;
    // else if (input.btn_down.is_down)
    //     line_end.y += memory.frame_time;
    // DrawLine(framebuffer, Vec2(0.1f, 0.1f), line_end);
    // for (int i = 0; i < 64; i++)
    // {
    //     float a = i / 63.0f;
    //     DrawLine(framebuffer, Vec2(0.1f, 0.1f), line_end + Vec2(a, 0.0f));
    // }
}
