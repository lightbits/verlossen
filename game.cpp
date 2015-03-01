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

uint32 PAL16[16] = {
    PAL16_VOID,
    PAL16_ASH,
    PAL16_BLIND,
    PAL16_BLOODRED,
    PAL16_PIGMEAT,
    PAL16_OLDPOOP,
    PAL16_NEWPOOP,
    PAL16_BLAZE,
    PAL16_ZORNSKIN,
    PAL16_SHADEGREEN,
    PAL16_LEAFGREEN,
    PAL16_SLIMEGREEN,
    PAL16_NIGHTBLUE,
    PAL16_SEABLUE,
    PAL16_SKYBLUE,
    PAL16_CLOUDBLUE
};

void DrawSprite(GameRenderer *render, GameTexture texture, int x, int y)
{
    render->Blit(texture,
        0, 0, texture.width, texture.height,
        x, y, texture.width, texture.height);
}

void LoadAssets(GameMemory &memory)
{
    memory.assets.bg   = memory.LoadTexture("plains.bmp");
    memory.assets.dude = memory.LoadTexture("dude_sw.bmp");
}

void GameUpdateAndRender(GameMemory &memory, GameInput &input)
{
    GameState    *state  = (GameState*)&memory;
    GameRenderer *render = (GameRenderer*)&memory.renderer;
    GameAssets   *assets = (GameAssets*)&memory.assets;

    if (!memory.is_initialized)
    {
        LoadAssets(memory);
        memory.is_initialized = true;
    }

    render->SetColor(PAL16_VOID);
    render->Clear();
    DrawSprite(render, assets->bg, 0, 0);

    internal vec2 cursor = Vec2(0.5f, 0.5f);
    if (input.btn_left.is_down)
        cursor.x -= 0.1f * memory.frame_time;
    else if (input.btn_right.is_down)
        cursor.x += 0.1f * memory.frame_time;
    if (input.btn_up.is_down)
        cursor.y -= 0.1f * memory.frame_time;
    else if (input.btn_down.is_down)
        cursor.y += 0.1f * memory.frame_time;

    // for (int c = 0; c < 16 ; c++)
    // {
    //     int mod = int(4.0f * memory.elapsed_time);
    //     render->SetColor(PAL16[(c + mod) % 16]);
    //     int len = 4;
    //     for (int run = 0; run < len; run++)
    //     {
    //         int x = c * len + run;
    //         render->DrawLine(x, 0, x, 1);
    //     }
    // }

    DrawSprite(render, assets->dude,
        int(cursor.x * render->res_x),
        int(cursor.y * render->res_y));
}
