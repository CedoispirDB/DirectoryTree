#include <stdlib.h>
#include "bitmap.h"
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
    img[idx + 1] = (color >> 8) & 0xFF;
    img[idx + 2] = color & 0xFF;
}

// void draw_rect(unsigned char *img, int w,
//                int x, int y, int rw, int rh)
// {
//     for (int i = 0; i < rw; i++)
//     {
//         set_pixel(img, w, x + i, y, 0, 0, 0);
//         set_pixel(img, w, x + i, y + rh, 0, 0, 0);
//     }
//     for (int i = 0; i < rh; i++)
//     {
//         set_pixel(img, w, x, y + i, 0, 0, 0);
//         set_pixel(img, w, x + rw, y + i, 0, 0, 0);
//     }
// }

void fill_rect(unsigned char *img, int w,
               int x, int y, int rw, int rh,
               unsigned int color)
{
    for (int j = 0; j < rh; j++)
        for (int i = 0; i < rw; i++)
            set_pixel(img, w, x + i, y + j, color);
}

void draw_line(unsigned char *img, int w,
               int x0, int y0, int x1, int y1)
{
    int dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
    int dy = -abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
    int err = dx + dy;

    while (1)
    {
        set_pixel(img, w, x0, y0, 0xFFFFFF);
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

void draw_arrow(unsigned char *img, int w,
                int x0, int y0, int x1, int y1)
{
    draw_line(img, w, x0, y0, x1, y1);
    draw_line(img, w, x1, y1, x1 - 5, y1 - 5);
    draw_line(img, w, x1, y1, x1 + 5, y1 - 5);
}

void draw_char(unsigned char *img, int w,
               int x, int y, char c)
{
    for (int row = 0; row < 8; row++)
    {
        unsigned char bits = font8x8[(unsigned char)c][row];

        for (int col = 0; col < 8; col++)
        {
            if ((bits << 1) & (1 << (7 - col)))
            {
                set_pixel(img, w,
                          x + col, y + row,
                          0x000000);
            }
        }
    }
}

void draw_text(unsigned char *img, int w,
               int x, int y, const char *s)
{
    while (*s)
    {
        draw_char(img, w, x, y, *s++);
        x += 8;
    }
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
                                  0x000000);
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

void draw_node(
    unsigned char *img,
    int w,
    int x, int y,
    const char *title, int scale,
    int padd,
    unsigned int color)
{
    int title_size = (int)strlen(title);
    int rw = BITMAP_SIZE * title_size * scale - 1 + padd * 2;
    int rh = BITMAP_SIZE - 1 + padd * 2;

    fill_rect(img, w, x, y, rw, rh, color);
    // -1 -> whitespace on the bitmap, +1 -> new line on the bitmap
    draw_text_scale(img, w, x + padd, y + padd, title, scale);
    draw_arrow(img, w, x + rw / 2, rh + 2, x + rw / 2, rh + 20);
}

int main()
{
    int w = 512;
    int h = 512;
    unsigned char *img = malloc(w * h * 3);
    for (int i = 0; i < w * h * 3; i++)
        img[i] = 255;

    char name[512] = "ROOT.TXT";
    
    // fill_rect(img, w, 8, 0, 8, 8, 255, 0, 0);
    draw_text_scale(img, w, 10, 10, name, 1);
    // draw_node(img, w, 0, 0, "ROOT", 1, 4, 0xFF0000);

    int ok = stbi_write_png("tree.png", w, h, 3, img, w * 3);
    if (!ok)
    {
        fprintf(stderr, "ERROR: FAILED TO WRITE PNG\n");
    }
    return 0;
}