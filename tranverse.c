#include <windows.h>
#include <stdio.h>
#include <stdbool.h>
#include <ctype.h>

#define IMG_UTIL_IMPLEMENTATION
#include "img_util.h"

#define IMG_WIDTH 1024
#define IMG_HEIGTH 1024

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
    int children_name_len;
};

typedef struct
{
    int parent_cnt;
} TreeData;

const char *basename(const char *path)
{
    const char *p = strrchr(path, '\\');
    return p ? p + 1 : path;
}

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
    new->children_name_len = 0;
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
    }
    else
    {
        Node *cur = parent->child;
        while (cur->sibling)
        {
            cur = cur->sibling;
        }
        cur->sibling = child;
    }

    parent->child_cnt++;
    parent->children_name_len += strlen(child->name);
}

void tranverse(const char *start_path, Node *root, TreeData *tree_data)
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
        // skip . files
        if (d.cFileName[0] == '.')
        {
            continue;
        }

        // Create new parent
        if (d.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
        {

            if (strcmp(d.cFileName, ".") != 0 &&
                strcmp(d.cFileName, "..") != 0)
            {
                snprintf(next, sizeof(next), "%s\\%s", start_path, d.cFileName);
                Node *new_root = new_node(d.cFileName, PARENT);
                tree_data->parent_cnt++;
                add_child(root, new_root);
                tranverse(next, new_root, tree_data);
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
    int x, int y,
    int max_needed_heigth,
    int scale, int internal_padd, int gap, int arrow_length, int lvl)
{
    if (!n)
    {
        return;
    }

    int next_level_expected_width = -1;
    int middle = -1;
    int start_children = -1;

    int title_size = (int)strlen(n->name);
    // -1 -> whitespace on the bitmap
    int rw = BITMAP_SIZE * title_size * scale - 1 + internal_padd * 2;
    int rh = BITMAP_SIZE - 1 + internal_padd * 2;

    // int wx = x - rw / 2;
    to_uppercase(n->name);

    if (n->type == PARENT)
    {
        // HANDLE ROOT
        if (x == IMG_WIDTH / 2)
        {
            x = x - rw / 2;
        }
        fill_rect(img, IMG_WIDTH, x, y, rw, rh, COLOR_RED);
        draw_text_scale(img, IMG_WIDTH, x + internal_padd, y + internal_padd, n->name, scale);
        draw_arrow(img,
                   IMG_WIDTH, x + rw / 2, y + rh + 2, x + rw / 2, y + rh + arrow_length);

        next_level_expected_width =
            n->children_name_len * BITMAP_SIZE * scale +
            ((internal_padd * 2) * n->child_cnt) - n->child_cnt + gap * (n->child_cnt - 1);
        middle = x + rw / 2;
        start_children = middle - next_level_expected_width / 2;
    }
    else
    {
        fill_rect(img, IMG_WIDTH, x, y, rw, rh, COLOR_YELLOW);
        draw_text_scale(img, IMG_WIDTH, x + internal_padd, y + internal_padd, n->name, scale);
    }

    printf("(%i - %s): start_children at: %i, parent at: %i\n", lvl, n->name, start_children, x);

    create_tree(
        n->child, img,
        start_children, y + arrow_length + 20 + 20,
        max_needed_heigth,
        scale, internal_padd, gap, arrow_length, lvl + 1);
    
    int next_sibling_pos = x + rw + internal_padd * 2 + gap;
    create_tree(
        n->sibling, img,
        next_sibling_pos, y,
        max_needed_heigth,
        scale, internal_padd, gap, arrow_length, lvl);
}

void prepare_tree(Node *root, TreeData tree_data, unsigned char *img)
{
    if (img == NULL || root == NULL)
    {
        fprintf(stderr, "ERROR: NULL PARAMETERS");
        return;
    }

    // white background
    for (int i = 0; i < IMG_WIDTH * IMG_HEIGTH * 3; i++)
    {
        img[i] = 255;
    }

    int scale = 1;
    int internal_padd = 3;
    int arrow_length = 20;
    int gap = 5;

    int max_needed_height = tree_data.parent_cnt * BITMAP_SIZE * scale + ((internal_padd * 2) * tree_data.parent_cnt) - tree_data.parent_cnt + gap * (tree_data.parent_cnt - 1);
    //  resize arrow acording to gap
    create_tree(root, img, IMG_WIDTH / 2, 0, max_needed_height, scale, internal_padd, gap, arrow_length, 0);
}

int main()
{
    const char *start_file = "C:\\Users\\marco\\Programming\\DirectoryTree";

    Node *root = NULL;
    TreeData tree_data = {.parent_cnt = 0};
    // save first parent
    if (root == NULL)
    {
        char name[512];
        const char *base = basename(start_file);
        strcpy(name, base);
        root = new_node(name, PARENT);
        tree_data.parent_cnt++;
    }

    tranverse(start_file, root, &tree_data);

    printf("\n\nTREE\n\n");
    printf("tree_data: %i\n", tree_data);
    walk(root, 0, false);

    int w = IMG_WIDTH;
    int h = IMG_HEIGTH;

    unsigned char *img = malloc(IMG_WIDTH * IMG_HEIGTH * 3);
    if (img == NULL)
    {
        fprintf(stderr, "ERROR: OUT OF MEMORY FOR IMG");
    }

    prepare_tree(root, tree_data, img);

    int ok = stbi_write_png("C:\\Users\\marco\\Programming\\DirectoryTree\\tree.png", w, h, 3, img, w * 3);
    if (!ok)
    {
        fprintf(stderr, "ERROR: FAILED TO WRITE PNG\n");
    }

    free(img);

    return 0;
}
