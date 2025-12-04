/**
 *
 * Elite - The New Kind.
 *
 * Allegro 5 version of Graphics routines.
 *
 * Ported from Allegro 4 to Allegro 5 with minimal behaviour changes.
 *
 **/

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <ctype.h>

#include <allegro5/allegro.h>
#include <allegro5/allegro_image.h>
#include <allegro5/allegro_primitives.h>
#include <allegro5/allegro_font.h>

#include "config.h"
#include "gfx.h"
#include "alg_data.h"
#include "elite.h"

/* ----------------------------------------------------------------------
 * Globals
 * --------------------------------------------------------------------*/

ALLEGRO_DISPLAY *gfx_display = NULL;
ALLEGRO_BITMAP  *gfx_screen  = NULL;   /* backbuffer / offscreen surface */
ALLEGRO_BITMAP  *scanner_image = NULL;

/* built-in font only (per user choice) */
static ALLEGRO_FONT *font_small = NULL;
static ALLEGRO_FONT *font_large = NULL;

/* sprite bitmaps (loaded from ./data) */
static ALLEGRO_BITMAP *sprite_grn_dot   = NULL;
static ALLEGRO_BITMAP *sprite_red_dot   = NULL;
static ALLEGRO_BITMAP *sprite_big_s     = NULL;
static ALLEGRO_BITMAP *sprite_elite_txt = NULL;
static ALLEGRO_BITMAP *sprite_big_e     = NULL;
static ALLEGRO_BITMAP *sprite_blake     = NULL;
static ALLEGRO_BITMAP *sprite_missile_g = NULL;
static ALLEGRO_BITMAP *sprite_missile_y = NULL;
static ALLEGRO_BITMAP *sprite_missile_r = NULL;

/* original code used frame_count + timer; here we use a simple
 * time-based frame cap instead (speed_cap is extern from config/main). */
extern int speed_cap;

/* for frame limiting */
static double last_frame_time = 0.0;

/* polygon sorting chain (unchanged logic) */
#define MAX_POLYS 100

static int start_poly;
static int total_polys;

struct poly_data
{
    int z;
    int no_points;
    int face_colour;
    int point_list[16];
    int next;
};

static struct poly_data poly_chain[MAX_POLYS];

/* anti-alias flag (extern from config.h) */
extern int anti_alias_gfx;

/* scanner filename (extern from alg_data.h) */
extern char scanner_filename[];

/* ----------------------------------------------------------------------
 * Small helpers
 * --------------------------------------------------------------------*/

/* Map old palette index (int) to an Allegro 5 color.
 * This is a rough mapping; adjust if you want the original palette exactly.
 */
static ALLEGRO_COLOR gfx_map_color(int col)
{
    switch (col)
    {
    case GFX_COL_BLACK:
        return al_map_rgb(0, 0, 0);
    case GFX_COL_WHITE:
        return al_map_rgb(255, 255, 255);
    case GFX_COL_GREY_2:
        return al_map_rgb(128, 128, 128);
    case GFX_COL_RED:
        return al_map_rgb(255, 0, 0);
    case GFX_COL_GREEN_1:
        return al_map_rgb(0, 255, 0);
    case GFX_COL_YELLOW_1:
        return al_map_rgb(255, 255, 0);
    default:
        return al_map_rgb(200, 200, 200);
    }
}

/* ensure built-in fonts are available */
static void gfx_ensure_fonts(void)
{
    if (!font_small)
        font_small = al_create_builtin_font();
    if (!font_large)
        font_large = font_small; /* simple: use same font for both sizes */
}

/* Helper to load a bitmap from ./data folder. */
static ALLEGRO_BITMAP *gfx_load_data_bitmap(const char *filename)
{
    char path[512];
    snprintf(path, sizeof(path), "data/%s", filename);
    return al_load_bitmap(path);
}

/* ----------------------------------------------------------------------
 * Startup / Shutdown
 * --------------------------------------------------------------------*/

int gfx_graphics_startup(void)
{
    int w = 800;
    int h = 600;

    if (!al_is_system_installed())
    {
        if (!al_init())
            return 1;
    }

    if (!al_is_image_addon_initialized())
        al_init_image_addon();

    if (!al_is_primitives_addon_initialized())
        al_init_primitives_addon();

    if (!al_is_font_addon_initialized())
        al_init_font_addon();

    /* Create display */
    gfx_display = al_create_display(w, h);
    if (!gfx_display)
    {
        fprintf(stderr, "Unable to create Allegro 5 display.\n");
        return 1;
    }

    /* Create the screen buffer bitmap */
    gfx_screen = al_create_bitmap(w, h);
    if (!gfx_screen)
    {
        fprintf(stderr, "Unable to create backbuffer bitmap.\n");
        return 1;
    }

    /* Load scanner image (from scanner_filename in alg_data.h, inside ./data/) */
    {
        char path[512];
        snprintf(path, sizeof(path), "data/%s", scanner_filename);
        scanner_image = al_load_bitmap(path);
        if (!scanner_image)
        {
            fprintf(stderr, "Error loading scanner bitmap: %s\n", path);
            return 1;
        }
    }

    /* Load sprites from ./data.
       NOTE: These filenames are guesses; rename your files to match or adjust here. */

    sprite_grn_dot   = gfx_load_data_bitmap("grndot.png");
    sprite_red_dot   = gfx_load_data_bitmap("reddot.png");
    sprite_big_s     = gfx_load_data_bitmap("safe.png");
    sprite_elite_txt = gfx_load_data_bitmap("elitetxt.png");
    sprite_big_e     = gfx_load_data_bitmap("ecm.png");
    sprite_blake     = gfx_load_data_bitmap("blake.png");
    sprite_missile_g = gfx_load_data_bitmap("missile_g.png");
    sprite_missile_y = gfx_load_data_bitmap("missile_y.png");
    sprite_missile_r = gfx_load_data_bitmap("missile_r.png");

    /* Backbuffer initial contents */
    al_set_target_bitmap(gfx_screen);
    al_clear_to_color(al_map_rgb(0, 0, 0));

    /* Draw scanner frame + border similar to old code */
    if (scanner_image)
    {
        al_draw_bitmap(scanner_image, GFX_X_OFFSET, 385 + GFX_Y_OFFSET, 0);
    }

    /* Top and left/right borders of the 512x384 main area */
    ALLEGRO_COLOR white = gfx_map_color(GFX_COL_WHITE);
    al_draw_line(0, 0, 0, 384, white, 1.0f);
    al_draw_line(0, 0, 511, 0, white, 1.0f);
    al_draw_line(511, 0, 511, 384, white, 1.0f);

    gfx_ensure_fonts();
    last_frame_time = al_get_time();

    return 0;
}


void gfx_graphics_shutdown(void)
{
    if (scanner_image)
    {
        al_destroy_bitmap(scanner_image);
        scanner_image = NULL;
    }

    if (sprite_grn_dot)   al_destroy_bitmap(sprite_grn_dot);
    if (sprite_red_dot)   al_destroy_bitmap(sprite_red_dot);
    if (sprite_big_s)     al_destroy_bitmap(sprite_big_s);
    if (sprite_elite_txt) al_destroy_bitmap(sprite_elite_txt);
    if (sprite_big_e)     al_destroy_bitmap(sprite_big_e);
    if (sprite_blake)     al_destroy_bitmap(sprite_blake);
    if (sprite_missile_g) al_destroy_bitmap(sprite_missile_g);
    if (sprite_missile_y) al_destroy_bitmap(sprite_missile_y);
    if (sprite_missile_r) al_destroy_bitmap(sprite_missile_r);

    sprite_grn_dot   = NULL;
    sprite_red_dot   = NULL;
    sprite_big_s     = NULL;
    sprite_elite_txt = NULL;
    sprite_big_e     = NULL;
    sprite_blake     = NULL;
    sprite_missile_g = NULL;
    sprite_missile_y = NULL;
    sprite_missile_r = NULL;

    if (gfx_screen)
    {
        al_destroy_bitmap(gfx_screen);
        gfx_screen = NULL;
    }

    if (gfx_display)
    {
        al_destroy_display(gfx_display);
        gfx_display = NULL;
    }

    if (font_small)
    {
        al_destroy_font(font_small);
        font_small = NULL;
    }

    if (font_large && font_large != font_small)
    {
        al_destroy_font(font_large);
        font_large = NULL;
    }
}

/* ----------------------------------------------------------------------
 * Screen update / locking
 * --------------------------------------------------------------------*/

/*
 * Blit the back buffer to the display, with simple frame cap.
 */
void gfx_update_screen(void)
{
    double now = al_get_time();
    double frame_len = (speed_cap > 0) ? (speed_cap / 1000.0) : 0.0;

    if (frame_len > 0 && (now - last_frame_time) < frame_len)
    {
        al_rest(frame_len - (now - last_frame_time));
        now = al_get_time();
    }
    last_frame_time = now;

    if (!gfx_display || !gfx_screen)
        return;

    al_set_target_backbuffer(gfx_display);
    al_clear_to_color(al_map_rgb(0, 0, 0));
    al_draw_bitmap(gfx_screen, 0, 0, 0);
    al_flip_display();
}

void gfx_acquire_screen(void)
{
    if (gfx_screen)
        al_set_target_bitmap(gfx_screen);
}

void gfx_release_screen(void)
{
    /* Allegro 5 doesn't need explicit release; no-op. */
}

/* ----------------------------------------------------------------------
 * Basic drawing primitives
 * --------------------------------------------------------------------*/

void gfx_fast_plot_pixel(int x, int y, int col)
{
    if (!gfx_screen) return;
    al_set_target_bitmap(gfx_screen);
    al_put_pixel(x, y, gfx_map_color(col));
}

void gfx_plot_pixel(int x, int y, int col)
{
    if (!gfx_screen) return;
    al_set_target_bitmap(gfx_screen);
    al_put_pixel(x + GFX_X_OFFSET, y + GFX_Y_OFFSET, gfx_map_color(col));
}

void gfx_draw_filled_circle(int cx, int cy, int radius, int circle_colour)
{
    if (!gfx_screen) return;
    al_set_target_bitmap(gfx_screen);
    al_draw_filled_circle(cx + GFX_X_OFFSET,
                          cy + GFX_Y_OFFSET,
                          (float)radius,
                          gfx_map_color(circle_colour));
}

/* We drop the original custom AA implementation and just degrade to normal
 * circle/line draw. The AA functions still exist for API compatibility. */

void gfx_draw_aa_circle(int cx, int cy, int radius_fixed)
{
    int radius = radius_fixed >> 16; /* radius was passed as itofix(radius) */
    gfx_draw_circle(cx, cy, radius, GFX_COL_WHITE);
}

void gfx_draw_aa_line(int x1_fixed, int y1_fixed, int x2_fixed, int y2_fixed)
{
    int x1 = x1_fixed >> 16;
    int y1 = y1_fixed >> 16;
    int x2 = x2_fixed >> 16;
    int y2 = y2_fixed >> 16;
    gfx_draw_colour_line(x1, y1, x2, y2, GFX_COL_WHITE);
}

void gfx_draw_circle(int cx, int cy, int radius, int circle_colour)
{
    if (!gfx_screen) return;
    al_set_target_bitmap(gfx_screen);

    ALLEGRO_COLOR col = gfx_map_color(circle_colour);

    al_draw_circle(cx + GFX_X_OFFSET,
                   cy + GFX_Y_OFFSET,
                   (float)radius,
                   col,
                   1.0f);
}

void gfx_draw_line(int x1, int y1, int x2, int y2)
{
    if (!gfx_screen) return;
    al_set_target_bitmap(gfx_screen);

    ALLEGRO_COLOR col = gfx_map_color(GFX_COL_WHITE);

    if (y1 == y2)
    {
        al_draw_line(x1 + GFX_X_OFFSET, y1 + GFX_Y_OFFSET,
                     x2 + GFX_X_OFFSET, y2 + GFX_Y_OFFSET, col, 1.0f);
        return;
    }

    if (x1 == x2)
    {
        al_draw_line(x1 + GFX_X_OFFSET, y1 + GFX_Y_OFFSET,
                     x2 + GFX_X_OFFSET, y2 + GFX_Y_OFFSET, col, 1.0f);
        return;
    }

    if (anti_alias_gfx)
        gfx_draw_aa_line(x1 << 16, y1 << 16, x2 << 16, y2 << 16);
    else
        al_draw_line(x1 + GFX_X_OFFSET, y1 + GFX_Y_OFFSET,
                     x2 + GFX_X_OFFSET, y2 + GFX_Y_OFFSET, col, 1.0f);
}

void gfx_draw_colour_line(int x1, int y1, int x2, int y2, int line_colour)
{
    if (!gfx_screen) return;
    al_set_target_bitmap(gfx_screen);

    ALLEGRO_COLOR col = gfx_map_color(line_colour);

    if (y1 == y2 || x1 == x2)
    {
        al_draw_line(x1 + GFX_X_OFFSET, y1 + GFX_Y_OFFSET,
                     x2 + GFX_X_OFFSET, y2 + GFX_Y_OFFSET, col, 1.0f);
        return;
    }

    if (anti_alias_gfx && (line_colour == GFX_COL_WHITE))
        gfx_draw_aa_line(x1 << 16, y1 << 16, x2 << 16, y2 << 16);
    else
        al_draw_line(x1 + GFX_X_OFFSET, y1 + GFX_Y_OFFSET,
                     x2 + GFX_X_OFFSET, y2 + GFX_Y_OFFSET, col, 1.0f);
}

void gfx_draw_triangle(int x1, int y1, int x2, int y2, int x3, int y3, int col)
{
    if (!gfx_screen) return;
    al_set_target_bitmap(gfx_screen);

    ALLEGRO_COLOR c = gfx_map_color(col);

    al_draw_filled_triangle(
        x1 + GFX_X_OFFSET, y1 + GFX_Y_OFFSET,
        x2 + GFX_X_OFFSET, y2 + GFX_Y_OFFSET,
        x3 + GFX_X_OFFSET, y3 + GFX_Y_OFFSET,
        c
    );
}

/* ----------------------------------------------------------------------
 * Text
 * --------------------------------------------------------------------*/

void gfx_display_text(int x, int y, char *txt)
{
    if (!gfx_screen) return;
    gfx_ensure_fonts();
    al_set_target_bitmap(gfx_screen);

    ALLEGRO_COLOR col = gfx_map_color(GFX_COL_WHITE);
    float fx = (float)((x / (2 / GFX_SCALE)) + GFX_X_OFFSET);
    float fy = (float)((y / (2 / GFX_SCALE)) + GFX_Y_OFFSET);

    al_draw_text(font_small, col, fx, fy, 0, txt);
}

void gfx_display_colour_text(int x, int y, char *txt, int col)
{
    if (!gfx_screen) return;
    gfx_ensure_fonts();
    al_set_target_bitmap(gfx_screen);

    ALLEGRO_COLOR c = gfx_map_color(col);
    float fx = (float)((x / (2 / GFX_SCALE)) + GFX_X_OFFSET);
    float fy = (float)((y / (2 / GFX_SCALE)) + GFX_Y_OFFSET);

    al_draw_text(font_small, c, fx, fy, 0, txt);
}

void gfx_display_centre_text(int y, char *str, int psize, int col)
{
    if (!gfx_screen) return;
    gfx_ensure_fonts();
    al_set_target_bitmap(gfx_screen);

    ALLEGRO_FONT *font = font_small;
    ALLEGRO_COLOR c = gfx_map_color(col);

    if (psize == 140)
    {
        /* mimic "bigger" text: still built-in, just same font now */
        font = font_large ? font_large : font_small;
        c = gfx_map_color(GFX_COL_WHITE);
    }

    float cx = (float)((128 * GFX_SCALE) + GFX_X_OFFSET);
    float cy = (float)((y / (2 / GFX_SCALE)) + GFX_Y_OFFSET);

    al_draw_text(font, c, cx, cy, ALLEGRO_ALIGN_CENTRE, str);
}

/* ----------------------------------------------------------------------
 * Clear / rectangles
 * --------------------------------------------------------------------*/

void gfx_clear_display(void)
{
    if (!gfx_screen) return;
    al_set_target_bitmap(gfx_screen);
    al_draw_filled_rectangle(GFX_X_OFFSET + 1, GFX_Y_OFFSET + 1,
                             510 + GFX_X_OFFSET, 383 + GFX_Y_OFFSET,
                             gfx_map_color(GFX_COL_BLACK));
}

void gfx_clear_text_area(void)
{
    if (!gfx_screen) return;
    al_set_target_bitmap(gfx_screen);
    al_draw_filled_rectangle(GFX_X_OFFSET + 1, GFX_Y_OFFSET + 340,
                             510 + GFX_X_OFFSET, 383 + GFX_Y_OFFSET,
                             gfx_map_color(GFX_COL_BLACK));
}

void gfx_clear_area(int tx, int ty, int bx, int by)
{
    if (!gfx_screen) return;
    al_set_target_bitmap(gfx_screen);
    al_draw_filled_rectangle(tx + GFX_X_OFFSET,
                             ty + GFX_Y_OFFSET,
                             bx + GFX_X_OFFSET,
                             by + GFX_Y_OFFSET,
                             gfx_map_color(GFX_COL_BLACK));
}

void gfx_draw_rectangle(int tx, int ty, int bx, int by, int col)
{
    if (!gfx_screen) return;
    al_set_target_bitmap(gfx_screen);
    al_draw_filled_rectangle(tx + GFX_X_OFFSET,
                             ty + GFX_Y_OFFSET,
                             bx + GFX_X_OFFSET,
                             by + GFX_Y_OFFSET,
                             gfx_map_color(col));
}

/* ----------------------------------------------------------------------
 * Pretty text (word-wrapped)
 * --------------------------------------------------------------------*/

void gfx_display_pretty_text(int tx, int ty, int bx, int by, char *txt)
{
    if (!gfx_screen) return;
    gfx_ensure_fonts();
    al_set_target_bitmap(gfx_screen);

    char strbuf[100];
    char *str = txt;
    char *bptr;
    int len = (int)strlen(txt);
    int pos;
    int maxlen;

    /* very approximate: assume 8px per char like old font */
    maxlen = (bx - tx) / 8;

    while (len > 0)
    {
        pos = maxlen;
        if (pos > len)
            pos = len;

        while ((str[pos] != ' ') && (str[pos] != ',') &&
               (str[pos] != '.') && (str[pos] != '\0') && pos > 0)
        {
            pos--;
        }

        len = len - pos - 1;

        for (bptr = strbuf; pos >= 0; pos--)
            *bptr++ = *str++;

        *bptr = '\0';

        al_draw_text(font_small, gfx_map_color(GFX_COL_WHITE),
                     tx + GFX_X_OFFSET, ty + GFX_Y_OFFSET, 0, strbuf);
        ty += (8 * GFX_SCALE);
    }
}

/* ----------------------------------------------------------------------
 * Scanner
 * --------------------------------------------------------------------*/

void gfx_draw_scanner(void)
{
    if (!gfx_screen || !scanner_image) return;
    al_set_target_bitmap(gfx_screen);
    al_draw_bitmap(scanner_image, GFX_X_OFFSET, 385 + GFX_Y_OFFSET, 0);
}

/* ----------------------------------------------------------------------
 * Clip region
 * --------------------------------------------------------------------*/

void gfx_set_clip_region(int tx, int ty, int bx, int by)
{
    if (!gfx_screen) return;
    al_set_target_bitmap(gfx_screen);
    al_set_clipping_rectangle(tx + GFX_X_OFFSET,
                              ty + GFX_Y_OFFSET,
                              bx - tx + 1,
                              by - ty + 1);
}

/* ----------------------------------------------------------------------
 * Polygon rendering / sorting
 * --------------------------------------------------------------------*/

void gfx_start_render(void)
{
    start_poly = 0;
    total_polys = 0;
}

void gfx_render_polygon(int num_points, int *point_list, int face_colour, int zavg)
{
    int i;
    int x;
    int nx;

    if (total_polys == MAX_POLYS)
        return;

    x = total_polys;
    total_polys++;

    poly_chain[x].no_points = num_points;
    poly_chain[x].face_colour = face_colour;
    poly_chain[x].z = zavg;
    poly_chain[x].next = -1;

    for (i = 0; i < 16; i++)
        poly_chain[x].point_list[i] = point_list[i];

    if (x == 0)
        return;

    if (zavg > poly_chain[start_poly].z)
    {
        poly_chain[x].next = start_poly;
        start_poly = x;
        return;
    }

    for (i = start_poly; poly_chain[i].next != -1; i = poly_chain[i].next)
    {
        nx = poly_chain[i].next;

        if (zavg > poly_chain[nx].z)
        {
            poly_chain[i].next = x;
            poly_chain[x].next = nx;
            return;
        }
    }

    poly_chain[i].next = x;
}

void gfx_render_line(int x1, int y1, int x2, int y2, int dist, int col)
{
    int point_list[4];

    point_list[0] = x1;
    point_list[1] = y1;
    point_list[2] = x2;
    point_list[3] = y2;

    gfx_render_polygon(2, point_list, col, dist);
}

void gfx_polygon(int num_points, int *poly_list, int face_colour)
{
    if (!gfx_screen) return;
    al_set_target_bitmap(gfx_screen);

    ALLEGRO_COLOR col = gfx_map_color(face_colour);

    float pts[32]; /* up to 16 points * 2 */
    int i;
    int x = 0;
    int y = 1;

    for (i = 0; i < num_points; i++)
    {
        int px = poly_list[x] + GFX_X_OFFSET;
        int py = poly_list[y] + GFX_Y_OFFSET;

        pts[i * 2 + 0] = (float)px;
        pts[i * 2 + 1] = (float)py;

        x += 2;
        y += 2;
    }

    al_draw_filled_polygon(pts, num_points, col);
}

void gfx_finish_render(void)
{
    int num_points;
    int *pl;
    int i;
    int col;

    if (total_polys == 0)
        return;

    for (i = start_poly; i != -1; i = poly_chain[i].next)
    {
        num_points = poly_chain[i].no_points;
        pl = poly_chain[i].point_list;
        col = poly_chain[i].face_colour;

        if (num_points == 2)
        {
            gfx_draw_colour_line(pl[0], pl[1], pl[2], pl[3], col);
            continue;
        }

        gfx_polygon(num_points, pl, col);
    }
}

/* ----------------------------------------------------------------------
 * Sprites
 * --------------------------------------------------------------------*/

void gfx_draw_sprite(int sprite_no, int x, int y)
{
    if (!gfx_screen) return;
    al_set_target_bitmap(gfx_screen);

    ALLEGRO_BITMAP *bmp = NULL;

    switch (sprite_no)
    {
    case IMG_GREEN_DOT:
        bmp = sprite_grn_dot;
        break;

    case IMG_RED_DOT:
        bmp = sprite_red_dot;
        break;

    case IMG_BIG_S:
        bmp = sprite_big_s;
        break;

    case IMG_ELITE_TXT:
        bmp = sprite_elite_txt;
        break;

    case IMG_BIG_E:
        bmp = sprite_big_e;
        break;

    case IMG_BLAKE:
        bmp = sprite_blake;
        break;

    case IMG_MISSILE_GREEN:
        bmp = sprite_missile_g;
        break;

    case IMG_MISSILE_YELLOW:
        bmp = sprite_missile_y;
        break;

    case IMG_MISSILE_RED:
        bmp = sprite_missile_r;
        break;

    default:
        return;
    }

    if (!bmp)
        return;

    int bw = al_get_bitmap_width(bmp);
    int bh = al_get_bitmap_height(bmp);

    if (x == -1)
        x = ((256 * GFX_SCALE) - bw) / 2;

    al_draw_bitmap(bmp, x + GFX_X_OFFSET, y + GFX_Y_OFFSET, 0);
}

/* ----------------------------------------------------------------------
 * File request
 * --------------------------------------------------------------------*/

/* Minimal stub: Allegro 5 has native file dialogs, but to keep things simple
   and avoid extra deps, we just return 0 (no file selected). */
int gfx_request_file(char *title, char *path, char *ext)
{
    (void)title;
    (void)path;
    (void)ext;
    return 0;
}
