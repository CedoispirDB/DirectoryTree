#include <windows.h>
#include <stdio.h>
#include <stdbool.h>
#include <ctype.h>

#define IMG_UTIL_IMPLEMENTATION
#include "img_util.h"

#define IMG_WIDTH 1024

typedef enum
{
    PARENT,
    CHILD
} NODE_TYPE;

typedef struct Node Node;
struct Node
{
    char name[512];
    Node *child;
    Node *sibling;
    int child_cnt;
    NODE_TYPE type;
};

Node *new_node(char *name, NODE_TYPE type)
{
    if (name == NULL)
    {
        fprintf(stderr, "ERROR: NODE NOT PROVIDED");
        return NULL;
    }

    Node *new = (Node *)malloc(sizeof(Node));
    if (!new)
    {
        fprintf(stderr, "ERROR: OUT OF MEMORY\n");
        return NULL;
    }

    strncpy(new->name, name, sizeof(new->name) - 1);
    new->name[sizeof(new->name) - 1] = '\0';
    new->child = NULL;
    new->sibling = NULL;
    new->child_cnt = 0;
    new->type = type;

    return new;
}

void add_child(Node *parent, Node *child)
{
    if (!parent || !child)
        return;

    if (!parent->child)
    {
        parent->child = child;
        parent->child_cnt++;
    }
    else
    {
        parent->child_cnt++;
        Node *cur = parent->child;
        while (cur->sibling)
        {
            cur = cur->sibling;
        }
        cur->sibling = child;
    }
}

void tranverse(const char *start_path, Node *root)
{
    WIN32_FIND_DATA d;
    HANDLE h;

    char search[512];
    char next[512];
    snprintf(search, sizeof(search), "%s\\*", start_path);

    h = FindFirstFile(search, &d);
    if (h == INVALID_HANDLE_VALUE)
    {
        fprintf(stderr, "ERROR: INVALID HANDLER\n");
        return;
    }

    do
    {
        // Create new parent
        if (d.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
        {

            if (strcmp(d.cFileName, ".") != 0 &&
                strcmp(d.cFileName, "..") != 0)
            {

                snprintf(next, sizeof(next), "%s\\%s", start_path, d.cFileName);
                Node *new_root = new_node(d.cFileName, PARENT);
                add_child(root, new_root);
                tranverse(next, new_root);
                SetCurrentDirectory("..");
            }
        }
        else
        {
            // add final children
            char new_child[512];
            strncpy(new_child, d.cFileName, sizeof(new_child) - 1);
            new_child[sizeof(new_child) - 1] = '\0';
            Node *child = new_node(new_child, CHILD);
            add_child(root, child);
        }
    } while (FindNextFile(h, &d));

    FindClose(h);
}

void walk(Node *n, int lvl, bool addr)
{
    if (!n)
        return;
    int pad = 5 * lvl;
    printf("%*s", pad, "");
    if (n->type == PARENT)
    {
        if (addr)
        {
            printf("[P]%s(%p) -> (%p, %p)\n", n->name, n, n->child, n->sibling);
        }
        else
        {
            printf("[P]%s(%i)\n", n->name, n->child_cnt);
        }
    }
    else
    {
        if (addr)
        {
            printf("%s%s(%p) -> (%p, %p)\n", "[C]", n->name, n, n->child, n->sibling);
        }
        else
        {
            printf("%s%s(%i)\n", "[C]", n->name, n->child_cnt);
        }
    }

    walk(n->child, lvl + 1, addr);
    walk(n->sibling, lvl, addr);
}

void to_uppercase(char *s)
{
    for (; *s; s++)
    {
        *s = (char)toupper((unsigned char)*s);
    }
}

void create_tree(
    Node *n, unsigned char *img,
    int x, int y)
{
    if (!n)
    {
        return;
    }

    int scale = 1;
    int padd = 3;
    int arrow_length = 20;

    int title_size = (int)strlen(n->name);
    // -1 -> whitespace on the bitmap
    int rw = BITMAP_SIZE * title_size * scale - 1 + padd * 2;
    int rh = BITMAP_SIZE - 1 + padd * 2;

    // int wx = x - rw / 2;
    to_uppercase(n->name);

    if (n->type == PARENT)
    {
        fill_rect(img, IMG_WIDTH, x, y, rw, rh, COLOR_RED);
        draw_text_scale(img, IMG_WIDTH, x + padd, y + padd, n->name, scale);
        draw_arrow(img,
                   IMG_WIDTH, x + rw / 2, y + rh + 2, x + rw / 2, y + rh + arrow_length);
    }
    else
    {
        fill_rect(img, IMG_WIDTH, x, y, rw, rh, COLOR_YELLOW);
        draw_text_scale(img, IMG_WIDTH, x + padd, y + padd, n->name, scale);
    }

    create_tree(n->child, img, x, y + arrow_length + 20 + 20);
    create_tree(n->sibling, img, x + title_size * BITMAP_SIZE + 50, y);
}

int main()
{
    const char *start_file = "C:\\Users\\marco\\Programming\\DirectoryTree";

    Node *root = new_node("DirectoryTree", PARENT);

    tranverse(start_file, root);

    printf("\n\nTREE\n\n");
    walk(root, 0, false);

    int w = IMG_WIDTH;
    int h = IMG_WIDTH;
    unsigned char *img = malloc(w * h * 3);
    if (img == NULL)
    {
        fprintf(stderr, "ERROR: OUT OF MEMORY FOR IMG");
        return 1;
    }

    for (int i = 0; i < w * h * 3; i++)
    {
        img[i] = 255;
    }

    create_tree(root, img, 0, 0);

    int ok = stbi_write_png("C:\\Users\\marco\\Programming\\DirectoryTree\\tree.png", w, h, 3, img, w * 3);
    if (!ok)
    {
        fprintf(stderr, "ERROR: FAILED TO WRITE PNG\n");
    }

    return 0;
}
