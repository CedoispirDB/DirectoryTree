#ifndef IMG_UTIL_H
#define IMG_UTIL_H

void set_pixel(
    unsigned char *img, int w, int x, int y,
    unsigned int color);

void fill_rect(
    unsigned char *img, int w,
    int x, int y, int rw, int rh,
    unsigned int color);

void draw_line(
    unsigned char *img, int w,
    int x0, int y0, int x1, int y1);

void draw_arrow(
    unsigned char *img, int w,
    int x0, int y0, int x1, int y1);

void draw_char_scale(
    unsigned char *img, int w,
    int x, int y, char c, int scale);

void draw_text_scale(
    unsigned char *img, int w,
    int x, int y, const char *s, int scale);

#ifdef IMG_UTIL_IMPLEMENTATION
#include <stdlib.h>
#include "bitmap.h"
#include "colors.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#define BITMAP_SIZE 8

void set_pixel(
    unsigned char *img, int w, int x, int y,
    unsigned int color)
{
    if (x < 0 || y < 0 || x >= w)
        return;
    int idx = (y * w + x) * 3;
    img[idx] = (color >> 16) & 0xFF;
    img[idx + 1] =  (color >> 8)  & 0xFF;
    img[idx + 2] = color        & 0xFF;
}

void fill_rect(
    unsigned char *img, int w,
    int x, int y, int rw, int rh,
    unsigned int color)
{
    for (int j = 0; j < rh; j++)
        for (int i = 0; i < rw; i++)
            set_pixel(img, w, x + i, y + j, color);
}

void draw_line(
    unsigned char *img, int w,
    int x0, int y0, int x1, int y1)
{
    int dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
    int dy = -abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
    int err = dx + dy;

    while (1)
    {
        set_pixel(img, w, x0, y0, COLOR_BLACK);
        if (x0 == x1 && y0 == y1)
            break;
        int e2 = 2 * err;
        if (e2 >= dy)
        {
            err += dy;
            x0 += sx;
        }
        if (e2 <= dx)
        {
            err += dx;
            y0 += sy;
        }
    }
}

void draw_arrow(
    unsigned char *img, int w,
    int x0, int y0, int x1, int y1)
{
    draw_line(img, w, x0, y0, x1, y1);
    draw_line(img, w, x1, y1, x1 - 5, y1 - 5);
    draw_line(img, w, x1, y1, x1 + 5, y1 - 5);
}

void draw_char_scale(unsigned char *img, int w,
                     int x, int y, char c, int scale)
{
    for (int row = 0; row < 8; row++)
    {
        unsigned char bits = font8x8[(unsigned char)c][row];

        for (int col = 0; col < 8; col++)
        {
            if ((bits << 1) & (1 << (7 - col)))
            {
                for (int dy = 0; dy < scale; dy++)
                {
                    for (int dx = 0; dx < scale; dx++)
                    {
                        set_pixel(img, w,
                                  x + col * scale + dx,
                                  y + row * scale + dy,
                                  COLOR_BLACK);
                    }
                }
            }
        }
    }
}

void draw_text_scale(unsigned char *img, int w,
                     int x, int y, const char *s, int scale)
{
    while (*s)
    {
        draw_char_scale(img, w, x, y, *s++, scale);
        x += (BITMAP_SIZE * scale);
    }
}

#endif /* IMG_UTIL_IMPLEMENTATION */
#endif /* IMG_UTIL_H */
