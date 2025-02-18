//
// Copyright 2023 Olivier Huguenot, Vadim Kindl
//
// Permission is hereby granted, free of charge, to any person obtaining a copy 
// of this software and associated documentation files (the “Software”), 
// to deal in the Software without restriction, including without limitation 
// the rights to use, copy, modify, merge, publish, distribute, sublicense, 
// and/or sell copies of the Software, and to permit persons to whom the 
// Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in 
// all copies or substantial portions of the Software.
// THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, 
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF 
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. 
// IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, 
// DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, 
// TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE 
// OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
//

#include "config.h"

#if ALIS_SDL_VER == 1

#include <SDL/SDL.h>

#include "sys.h"
#include "alis.h"
#include "audio.h"
#include "channel.h"
#include "image.h"
#include "mem.h"
#include "platform.h"
#include "utils.h"

#include "emu2149.h"
#include "math.h"


const u32               k_event_ticks = (1000 / 120);  // poll keyboard 120 times per second
const u32               k_frame_ticks = (1000 / 50);   // 50 fps screen update

SDL_Surface             *surface;

extern SDL_keysym       button;
extern SDL_Event        event;
extern SDL_AudioSpec    *audio_spec;
extern SDL_Rect         dirty_rects[2048];
extern SDL_Rect         dirty_mouse_rect;

extern u8               shift;
extern mouse_t          mouse;
extern bool             dirty_mouse;
extern u32              width;
extern u32              height;

extern int              audio_id;

extern u8               fls_ham6;
extern u8               fls_s512;
extern u8               *vgalogic_df;

extern u32              poll_ticks;
extern u32              frame_time;
extern u32              loop_time;

extern double           isr_step;
extern double           isr_counter;

u8                      *prev_pixels = NULL;

u8                      dirty_pal = 1;
s32                     dirty_len = 0;
u32                     tmp_pal[256];

void sys_audio_callback(void *userdata, u8 *stream, s32 len);

// ============================================================================
#pragma mark - SDL1 System init
// ============================================================================

void sys_init(sPlatform *pl, int fullscreen, int mutesound) {

    dirty_mouse_rect = (SDL_Rect){0, 0, 16, 16};
    
    width = pl->width;
    height = pl->height;
    
    printf("  SDL initialization...\n");
    if (SDL_Init(SDL_INIT_VIDEO|SDL_INIT_AUDIO) < 0) {
        fprintf(stderr, "   Unable to initialize SDL: %s\n", SDL_GetError());
        exit(-1);
    }
    
    printf("  Video initialization...\n");
    
    SDL_WM_SetCaption(kProgName, "icon");
    surface = SDL_SetVideoMode(width, height, 0, SDL_SWSURFACE | SDL_RESIZABLE);
    if (surface == NULL) {
        fprintf(stderr, "   Could not create %.0dx%.0d window: %s\n", (width), (height), SDL_GetError());
        exit(-1);
    }
    
    prev_pixels = surface->pixels;

    dirty_mouse = 0;
    
    printf("  Audio initialization...\n");
    audio_spec = (SDL_AudioSpec *)malloc(sizeof(SDL_AudioSpec));
    
    SDL_AudioSpec *desired_spec = (SDL_AudioSpec *)malloc(sizeof(SDL_AudioSpec));
    
    memset(desired_spec, 0, sizeof(SDL_AudioSpec));
    desired_spec->freq = 12500;
    desired_spec->format = AUDIO_S16MSB;
    desired_spec->channels = 1;
    desired_spec->samples = desired_spec->freq / 20; // 20 ms
    desired_spec->callback = sys_audio_callback;
    desired_spec->userdata = NULL;
    desired_spec->size = desired_spec->samples;
    
    memcpy(audio_spec, desired_spec, sizeof(SDL_AudioSpec));
    
//    if (SDL_OpenAudio(desired_spec, audio_spec) != 0) {
    if (SDL_OpenAudio(desired_spec, 0) != 0) {
        fprintf(stderr, "   Could not open audio: %s\n", SDL_GetError());
        free(audio_spec);
        surface->pixels = prev_pixels;
        SDL_FreeSurface(surface);
        SDL_Quit();
        exit(-1);
    }
    
    // Extended information on obtained audio for tests and reports
    printf("  Opened audio device id %d successfully:\n", audio_id);
    printf("    Frequency:  %d\n", audio_spec->freq);
    printf("    Format:     0x%04x\n", audio_spec->format);
    printf("    Channels:   %d\n", audio_spec->channels);
    printf("    Samples:    %d\n", audio_spec->samples);
    printf("    Silence:    %d\n", audio_spec->silence);
    printf("    Padding:    %d\n", audio_spec->padding);
    printf("    Size:       %d\n", audio_spec->size);
    
    free(desired_spec);

    audio.host_freq = audio_spec->freq;
    audio.host_format = audio_spec->format;

//    sys_init_psg();
    
    isr_step = (double)50.0 / (double)(audio_spec->freq);
    isr_counter = 1;
    
    poll_ticks = SDL_GetTicks();
    frame_time = SDL_GetTicks();
    loop_time = SDL_GetTicks();

    SDL_PauseAudio(mutesound);
}

void sys_init_timers(void) {
    
    poll_ticks = SDL_GetTicks();
    frame_time = SDL_GetTicks();
    loop_time = SDL_GetTicks();
}

void sys_sleep_until(u32 *start, s32 len)
{
    u32 now = SDL_GetTicks();
    s32 wait = len - (now - *start);
    if (wait > 0 && wait < 100)
        SDL_Delay(wait);
    *start = now;
}

void sys_sleep_interactive(s32 *loop, s32 intr)
{
    u32 prev = SDL_GetTicks();
    while (*loop > intr || (*loop > 0 && io_inkey() == 0))
    {
#if ALIS_USE_THREADS <= 0
        sys_poll_event();
#endif

        SDL_Delay(k_event_ticks);

        u32 now = SDL_GetTicks();
        *loop -= now - prev;
        prev = now;
    }

    *loop = 0;
}

void sys_delay_loop(void)
{
    sys_sleep_until(&loop_time, 6);
}

void sys_delay_frame(void)
{
    sys_sleep_until(&frame_time, k_frame_ticks * alis.ctiming);
}

void sys_sleep_until_music_stops(void)
{
    if (SDL_GetAudioStatus() != SDL_AUDIO_PLAYING)
        return;

    audio.working = 1;

    for (int count = 0; count < 10 && audio.muflag; count++)
    {
        SDL_Delay(10);
    }

    audio.muflag = 0;
    audio.musicId = 0xffff;

    for (int count = 0; count < 10 && audio.working; count++)
    {
        SDL_Delay(10);
    }
}

// ============================================================================
#pragma mark - SDL1 Main system loop
// ============================================================================

u8 sys_start(void) {

    alis.state = eAlisStateRunning;
    alis_thread(NULL);
    return 0;
}

void sys_poll_event(void) {

    sys_sleep_until(&poll_ticks, k_frame_ticks);
    
    itroutine(20, NULL);
    
    SDL_PollEvent(&event);
    switch (event.type) {
            
        case SDL_QUIT:
        {
            printf("\n");
            ALIS_DEBUG(EDebugSystem, "SDL: Quit.\n");
            ALIS_DEBUG(EDebugSystem, "A STOP signal has been sent to the VM queue...\n");
            alis.state = eAlisStateStopped;
            break;
        }
        case SDL_KEYUP:
        {
            switch (event.key.keysym.sym)
            {
                case SDLK_LALT:
                    printf("\n");
                    ALIS_DEBUG(EDebugSystem, "INTERRUPT: User debug label.\n");
                    break;

                case SDLK_F11:
                    alis.state = eAlisStateSave;
                    break;

                case SDLK_F12:
                    alis.state = eAlisStateLoad;
                    break;
                    
                default:
                    break;
            }

            break;
        }
        case SDL_KEYDOWN:
        {
            if (event.key.keysym.mod == KMOD_RSHIFT || event.key.keysym.mod == KMOD_LSHIFT)
            {
                shift = 1;
            }

            if (event.key.keysym.sym == SDLK_F11 || event.key.keysym.sym == SDLK_F12)
            {
                break;
            }

            if (event.key.keysym.sym == SDLK_PAUSE)
            {
                printf("\n");
                ALIS_DEBUG(EDebugSystem, "INTERRUPT: Quit by user request.\n");
                ALIS_DEBUG(EDebugSystem, "A STOP signal has been sent to the VM queue...\n");
                alis.state = eAlisStateStopped;
            }
            else
            {
                button = event.key.keysym;
            }
            
            break;
        }
    };
    
    // update mouse
    dirty_rects[dirty_len] = dirty_mouse_rect;
    dirty_len++;

    u32 bt = SDL_GetMouseState(&mouse.x, &mouse.y);
    
    mouse.lb = SDL_BUTTON(bt) == SDL_BUTTON_LEFT;
    mouse.rb = SDL_BUTTON(bt) == SDL_BUTTON_RIGHT;

    dirty_rects[dirty_len] = dirty_mouse_rect = (SDL_Rect){ .x = min(max(0, mouse.x), 304), .y = min(max(0, mouse.y), 184), .w = 16, .h = 16 };
    dirty_len++;
    
    sys_render(host.pixelbuf);
}

// ============================================================================
#pragma mark - SDL1 Video
// ============================================================================

void setHAMto32bit(void *pixels, u32 px, u8 *rawcolor) {
    ((u32 *)pixels)[px] = (u32)(0xff000000 + (rawcolor[0] << 16) + (rawcolor[1] << 8) + (rawcolor[2]));
}

void setHAMto16bit(void *pixels, u32 px, u8 *rawcolor) {
    ((u16 *)pixels)[px] = (u16)((rawcolor[0] >> 3) << 11) + ((rawcolor[1] >> 2) << 5) + (rawcolor[2] >> 3);
}

void setHAMto8bit(void *pixels, u32 px, u8 *rawcolor) {
    ((u8 *)pixels)[px] = (u8)(((rawcolor[0] >> 6) << 6) + ((rawcolor[1] >> 5) << 3) + (((rawcolor[2] >> 5))));
}

void set9to32bit(void *pixels, u32 px, u8 *rawcolor) {
    ((u32 *)pixels)[px] = (u32)(0xff000000 + (((rawcolor[0] & 0b00000111) << 5) << 16) + (((rawcolor[1] >> 4) << 5) << 8) + (((rawcolor[1] & 0b00000111) << 5)));
}

void set9to16bit(void *pixels, u32 px, u8 *rawcolor) {
    ((u16 *)pixels)[px] = (u16)((((rawcolor[0] & 0b00000111) << 2) << 11) + (((rawcolor[1] >> 4) << 3) << 5) + (((rawcolor[1] & 0b00000111) << 2)));
}

void set9to8bit(void *pixels, u32 px, u8 *rawcolor) {
    ((u8 *)pixels)[px] = (u8)((((rawcolor[0] & 0b00000111) >> 1) << 6) + ((rawcolor[1] >> 4) << 3) + (((rawcolor[1] & 0b00000111))));
}

void sys_render(pixelbuf_t buffer) {

    if (dirty_len == 0 && dirty_pal == 0)
        return;
    
    if (dirty_mouse)
    {
        dirty_mouse = false;
        sys_dirty_mouse();
    }

    if (fls_ham6)
    {
        u8 *bitmap = vgalogic_df + 0xa0;
        u32 *pixels = surface->pixels;

        // Amiga HAM bitplanes

        SDL_Color sdl_palette[256];

        void (*to_pixels)(void *, u32, u8 *) = &setHAMto32bit;

        switch (surface->format->BitsPerPixel)
        {
            case 32:
            {
                to_pixels = &setHAMto32bit;
                break;
            }
            case 16:
            {
                to_pixels = &setHAMto16bit;
                break;
            }
            case 8:
            {
                for (int i = 0; i < 256; i++)
                {
                    sdl_palette[i].r = (i & 0b11000000);
                    sdl_palette[i].g = (i & 0b00111000) << 2;
                    sdl_palette[i].b = (i & 0b00000111) << 5;
                }

                SDL_SetPalette(surface, SDL_PHYSPAL, sdl_palette, 0, 256);

                to_pixels = &setHAMto8bit;
                break;
            }
        };

        u32 px = 0;
        u32 planesize = (buffer.w * buffer.h) / 8;
        u8 c0, c1, c2, c3, c4, c5;
        u8 rawcolor[3];

        for (s32 h = 0; h < buffer.h; h++)
        {
            u8 *tgt = buffer.data + (h * buffer.w);
            for (s32 w = 0; w < buffer.w; w++, tgt++, px++)
            {
                s32 idx = (w + h * buffer.w) / 8;
                c0 = *(bitmap + idx);
                c1 = *(bitmap + (idx += planesize));
                c2 = *(bitmap + (idx += planesize));
                c3 = *(bitmap + (idx += planesize));
                c4 = *(bitmap + (idx += planesize));
                c5 = *(bitmap + (idx += planesize));

                int bit = 7 - (w % 8);
                
                int control = ((c4 >> bit) & 1) << 0 | ((c5 >> bit) & 1) << 1;
                int index = ((c0 >> bit) & 1) | ((c1 >> bit) & 1) << 1 | ((c2 >> bit) & 1) << 2 | ((c3 >> bit) & 1) << 3;

                switch (control) {
                    case 0:
                        rawcolor[0] = buffer.palette[(index << 2) + 0];
                        rawcolor[1] = buffer.palette[(index << 2) + 1];
                        rawcolor[2] = buffer.palette[(index << 2) + 2];
                        break;
                    case 1:
                        rawcolor[2] = index << 4;
                        break;
                    case 2:
                        rawcolor[0] = index << 4;
                        break;
                    case 3:
                        rawcolor[1] = index << 4;
                        break;
                }

                to_pixels(pixels, px, rawcolor);
            }
        }
    }
    else if (fls_s512)
    {
        dirty_rects[0] = (SDL_Rect){ .x = 0, .y = 0, .w = host.pixelbuf.w, .h = host.pixelbuf.h };
        dirty_len = 1;

        SDL_Color sdl_palette[256];

        void (*to_pixels)(void *, u32, u8 *) = &set9to32bit;

        switch (surface->format->BitsPerPixel)
        {
            case 32:
            {
                to_pixels = &set9to32bit;
                break;
            }
            case 16:
            {
                to_pixels = &set9to16bit;
                break;
            }
            case 8:
            {
                for (int i = 0; i < 256; i++)
                {
                    sdl_palette[i].r = (i & 0b11000000);
                    sdl_palette[i].g = (i & 0b00111000) << 2;
                    sdl_palette[i].b = (i & 0b00000111) << 5;
                }

                SDL_SetPalette(surface, SDL_PHYSPAL, sdl_palette, 0, 256);

                to_pixels = &set9to8bit;
                break;
            }
        };
        
        u8 *bitmap = vgalogic_df + 0xa0;
        u8 *pixels = surface->pixels;

        // Atari ST Spectrum 512 bitplanes
        
        u16 curpal[16];
        memcpy(curpal, vgalogic_df + 32000, 32);

        // s16 pallines = 0xc5 - fls_pallines;

        u16 *palette = (u16 *)(vgalogic_df + 32000 + 32);

        // ST scanline width 48 + 320 + 44 (412)
        // change palette every 412 / 48 ?
        float pxs = 9.6;

        int limit0 = -4;
        int limit1 = 156;
        
        u32 px = 0;
        u32 at = 0;

        u32 palidx;
        u8 *rawcolor;
        
        u32 rot;
        u32 mask, iat;

        for (int y = 0; y < buffer.h; y++, palette += 16)
        {
            int lpx = 0;
            
            for (int x = 0; x < buffer.w; x+=16, at+=8)
            {
                for (int i = 0; i < 2; i++)
                {
                    iat = at + i;
                    for (int dpx = 0; dpx < 8; dpx++, lpx++, px++)
                    {
                        if (lpx >= limit0)
                        {
                            if (lpx == limit1)
                                palette += 16;
                            
                            palidx = lpx < limit1 ? (lpx - limit0) / pxs : (lpx - limit1) / pxs;
                            if (palidx < 16)
                                curpal[palidx] = palette[palidx];
                        }
                        
                        rot = (7 - dpx);
                        mask = 1 << rot;
                        
                        rawcolor = (u8 *)&(curpal[(((bitmap[iat + 0] & mask) >> rot) << 0) | (((bitmap[iat + 2] & mask) >> rot) << 1) | (((bitmap[iat + 4] & mask) >> rot) << 2) | (((bitmap[iat + 6] & mask) >> rot) << 3)]);
                        to_pixels(pixels, px, rawcolor);
                    }
                }
            }
            
            palette += 16;
            memcpy(curpal, palette, 32);
        }

    }
    else
    {
        if (dirty_pal)
        {
            switch (surface->format->BitsPerPixel)
            {
                case 32:
                {
                    for (int i = 0; i < 1024; i+=4)
                        tmp_pal[i>>2] = (u32)(0xff000000 + (buffer.palette[i + 0] << 16) + (buffer.palette[i + 1] << 8) + (buffer.palette[i + 2] << 0));

                    dirty_rects[0] = (SDL_Rect){ .x = 0, .y = 0, .w = host.pixelbuf.w, .h = host.pixelbuf.h };
                    dirty_len = 1;
                    break;
                }
                case 16:
                {
                    u16 *tmppal16 = (u16 *)tmp_pal;
                    for (int i = 0; i < 1024; i+=4)
                        tmppal16[i>>2] = (u16)(((buffer.palette[i + 0] >> 3) << 11) + ((buffer.palette[i + 1] >> 2) << 5) + ((buffer.palette[i + 2] >> 3) << 0));

                    dirty_rects[0] = (SDL_Rect){ .x = 0, .y = 0, .w = host.pixelbuf.w, .h = host.pixelbuf.h };
                    dirty_len = 1;
                    break;
                }
                case 8:
                {
                    SDL_SetPalette(surface, SDL_PHYSPAL, (SDL_Color *)buffer.palette, 0, 256);
                    break;
                }
            };
            
            dirty_pal = 0;
        }
        
        if (surface->format->BitsPerPixel == 8)
        {
            surface->pixels = buffer.data;
        }
        else
        {
            for (int i = 0; i < dirty_len; i++)
            {
                SDL_Rect r = dirty_rects[i];
                switch (surface->format->BitsPerPixel)
                {
                    case 32:
                    {
                        u32 *pixels = surface->pixels;
                        
                        int px = r.x + r.y * buffer.w;
                        int po = buffer.w - r.w;
                        for (int y = r.y; y < r.y + r.h; y++, px += po)
                        {
                            for (int x = 0; x < r.w; x++, px++)
                            {
                                pixels[px] = tmp_pal[buffer.data[px]];
                            }
                        }
                        
                        break;
                    }
                    case 16:
                    {
                        u16 *tmppal16 = (u16 *)tmp_pal;
                        u16 *pixels = surface->pixels;

                        int px = r.x + r.y * buffer.w;
                        int po = buffer.w - r.w;
                        for (int y = r.y; y < r.y + r.h; y++, px += po)
                        {
                            for (int x = 0; x < r.w; x++, px++)
                            {
                                pixels[px] = tmppal16[buffer.data[px]];
                            }
                        }
                        break;
                    }
                };
            }
        }
    }
    
    SDL_UpdateRects(surface, abs(dirty_len), dirty_rects);
    dirty_len = 0;
}

// ============================================================================
#pragma mark - SDL1 System deinit
// ============================================================================

void sys_deinit(void) {
    
    SDL_Delay(20);   // 20ms fail-safe delay to make sure that sound buffer is empty

    sys_deinit_psg();

    free(audio_spec);
    
    surface->pixels = prev_pixels;
    SDL_FreeSurface(surface);

    SDL_Quit();
}

void sys_dirty_mouse(void) {
    
    SDL_ShowCursor(mouse.enabled);
}

// =============================================================================
#pragma mark - SDL1 SYNC
// =============================================================================

void sys_lock_renderer(void) {
}

void sys_unlock_renderer(void) {
}

#endif
