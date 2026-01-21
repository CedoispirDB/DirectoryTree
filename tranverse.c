#include <windows.h>
#include <stdio.h>
#include <stdbool.h>
#include <ctype.h>

#define IMG_UTIL_IMPLEMENTATION
#include "img_util.h"

#define IMG_WIDTH 1024
#define IMG_HEIGHT 1024

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
    NODE_TYPE type;
    int child_cnt;
    int children_name_len;
};

typedef struct DrawNode DrawNode;
struct DrawNode
{
    char name[512];
    int child_cnt;
    DrawNode *child;
    DrawNode *sibling;
    NODE_TYPE type;

    int draw_x;
    int draw_y;
    int draw_width;
    int draw_heigth;
    unsigned int color;
    bool has_gap;
    bool is_first_child;
    int next_level_needed_width;
};

typedef struct
{
    DrawNode *node;
    int max_width_needed;
    int max_height_needed;
    int parent_cnt;
    int scale;
    int internal_padd;
    int arrow_length;
    int gap;
} TreeData;

const char *basename(const char *path)
{
    const char *p = strrchr(path, '\\');
    return p ? p + 1 : path;
}

void to_uppercase(char *s)
{
    for (; *s; s++)
    {
        *s = (char)toupper((unsigned char)*s);
    }
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

void free_node(Node *n)
{
    if (!n)
        return;

    free_node(n->child);
    free_node(n->sibling);
    free(n);
}

DrawNode *new_draw_node(char *name, NODE_TYPE type)
{
    if (name == NULL)
    {
        fprintf(stderr, "ERROR: DrawNode NOT PROVIDED");
        return NULL;
    }

    DrawNode *new = (DrawNode *)malloc(sizeof(DrawNode));
    if (!new)
    {
        fprintf(stderr, "ERROR: OUT OF MEMORY\n");
        return NULL;
    }

    strncpy(new->name, name, sizeof(new->name) - 1);
    new->name[sizeof(new->name) - 1] = '\0';
    to_uppercase(new->name);
    new->child = NULL;
    new->sibling = NULL;
    new->type = type;
    new->draw_x = 0;
    new->draw_y = 0;
    new->draw_width = 0;
    new->draw_heigth = 0;
    new->color = 0;
    new->child_cnt = 0;
    new->has_gap = false;
    new->is_first_child = false;
    new->next_level_needed_width = 0;
    return new;
}

void add_draw_child(DrawNode *parent, DrawNode *child)
{
    if (!parent || !child)
        return;

    if (!parent->child)
    {
        parent->child = child;
    }
    else
    {
        DrawNode *cur = parent->child;
        while (cur->sibling)
        {
            cur = cur->sibling;
        }
        cur->sibling = child;
    }
}

void free_draw_tree(DrawNode *n)
{
    if (!n)
        return;

    free_draw_tree(n->child);
    free_draw_tree(n->sibling);
    free(n);
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

// Show tree in terminal
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
            printf("[P]%s\n", n->name);
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
            printf("%s%s\n", "[C]", n->name);
        }
    }

    walk(n->child, lvl + 1, addr);
    walk(n->sibling, lvl, addr);
}

void walk_draw(DrawNode *n, int lvl, bool addr)
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
            printf("[P]%s\n", n->name);
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
            printf("%s%s\n", "[C]", n->name);
        }
    }

    walk_draw(n->child, lvl + 1, addr);
    walk_draw(n->sibling, lvl, addr);
}

void walk_draw_verbose(DrawNode *n, int lvl, bool addr)
{
    if (!n)
        return;

    int pad = 4 * lvl;
    printf("%*s", pad, "");

    /* header do nó */
    printf("[%s] name=\"%s\"",
           (n->type == PARENT) ? "P" : "C",
           n->name);

    if (addr)
    {
        printf(" @%p", (void *)n);
    }
    printf("\n");

    /* campos internos */
    printf("%*s  child_cnt        = %d\n", pad, "", n->child_cnt);
    printf("%*s  type             = %d\n", pad, "", n->type);

    printf("%*s  draw_x           = %d\n", pad, "", n->draw_x);
    printf("%*s  draw_y           = %d\n", pad, "", n->draw_y);
    printf("%*s  draw_width       = %d\n", pad, "", n->draw_width);
    printf("%*s  draw_heigth      = %d\n", pad, "", n->draw_heigth);
    printf("%*s  has_gap          = %d\n", pad, "", n->has_gap);
    printf("%*s  is_first_child   = %d\n", pad, "", n->is_first_child);
    printf("%*s  color            = 0x%06X\n", pad, "", n->color);

    if (addr)
    {
        printf("%*s  child           = %p\n", pad, "", (void *)n->child);
        printf("%*s  sibling         = %p\n", pad, "", (void *)n->sibling);
    }

    printf("\n");

    /* desce na árvore */
    walk_draw_verbose(n->child, lvl + 1, addr);
    walk_draw_verbose(n->sibling, lvl, addr);
}

// Prepare data for drawing tree
void prepare_drawing_tree(
    Node *source, DrawNode *draw_root, TreeData *tree_data,
    int draw_x, int draw_y,
    bool is_first_child)
{
    if (!source || !tree_data)
    {
        return;
    }

    if (draw_x < 0 || draw_y < 0 || draw_x >= IMG_WIDTH || draw_y >= IMG_HEIGHT)
    {
        return;
    }

    int next_level_expected_width = -1;
    int middle = -1;
    int start_children = -1;

    int title_size = (int)strlen(source->name);
    // -1 -> whitespace on the bitmap
    int rw = BITMAP_SIZE * title_size * tree_data->scale - 1 + tree_data->internal_padd * 2;
    int rh = BITMAP_SIZE - 1 + tree_data->internal_padd * 2;

    DrawNode *new_node = new_draw_node(source->name, source->type);
    if (new_node->type == PARENT)
    {
        // HANDLE ROOT
        if (draw_x == IMG_WIDTH / 2)
        {
            draw_x = draw_x - rw / 2;
            tree_data->node = new_node;
        }

        int gap_num = (source->child_cnt > 0) ? (source->child_cnt - 1) : 0;
        int total_gap = tree_data->gap * gap_num;
        next_level_expected_width =
            source->children_name_len * BITMAP_SIZE * tree_data->scale +
            ((tree_data->internal_padd * 2) * source->child_cnt) - source->child_cnt;

        if (next_level_expected_width + total_gap > IMG_WIDTH)
        {
            printf("WARNING: Gap exceeded screen width. Resizing.\n");
            int new_gap = (IMG_WIDTH - next_level_expected_width + source->child_cnt) / gap_num;
            tree_data->gap = new_gap > 0 ? new_gap : 0;
        }

        middle = draw_x + rw / 2;
        start_children = middle - next_level_expected_width / 2; // - tree_data->gap * gap_num
        tree_data->parent_cnt = tree_data->parent_cnt + 1;

        new_node->next_level_needed_width = next_level_expected_width;
        new_node->color = COLOR_RED;
    }
    else
    {
        new_node->color = COLOR_YELLOW;
    }

    new_node->draw_x = draw_x;
    new_node->draw_y = draw_y;
    new_node->draw_width = rw;
    new_node->draw_heigth = rh;
    new_node->child_cnt = source->child_cnt;
    new_node->is_first_child = is_first_child;

    if (source->sibling != NULL)
    {
        new_node->has_gap = true;
    }

    add_draw_child(draw_root, new_node);

    prepare_drawing_tree(
        source->child, new_node, tree_data,
        start_children, draw_y + rh + tree_data->arrow_length + 40,
        true);

    int next_sibling_pos = draw_x + rw; // + tree_data->gap
    prepare_drawing_tree(
        source->sibling, draw_root, tree_data,
        next_sibling_pos, draw_y,
        false);
}

void draw_tree(
    unsigned char *img, DrawNode *d, TreeData tree_data,
    int x, int y // comes from parent
)
{
    if (!img || !d)
    {
        return;
    }

    int start_children = 0;

    fill_rect(img, IMG_WIDTH, x, d->draw_y, d->draw_width, d->draw_heigth, d->color);
    draw_text_scale(img, IMG_WIDTH, x + tree_data.internal_padd, d->draw_y + tree_data.internal_padd, d->name, tree_data.scale);
    if (d->type == PARENT)
    {

        draw_arrow(
            img, IMG_WIDTH,
            x + d->draw_width / 2, d->draw_y + d->draw_heigth + 2,
            x + d->draw_width / 2, d->draw_y + d->draw_heigth + tree_data.arrow_length);

        int gap_cnt = (d->child_cnt > 0) ? (d->child_cnt - 1) : 0;
        int middle = x + d->draw_width / 2;
        start_children = middle - (d->next_level_needed_width + tree_data.gap * (gap_cnt)) / 2;
    }

    draw_tree(
        img, d->child, tree_data,
        start_children, y);
    int next_sibling_pos = x + d->draw_width + tree_data.gap;
    draw_tree(
        img, d->sibling, tree_data,
        next_sibling_pos, y);
}

void load_tree(Node *root, TreeData *tree_data, unsigned char *img)
{
    if (img == NULL || root == NULL)
    {
        fprintf(stderr, "ERROR: NULL PARAMETERS");
        return;
    }

    // white background
    for (int i = 0; i < IMG_WIDTH * IMG_HEIGHT * 3; i++)
    {
        img[i] = 255;
    }

    // INIT TREE DATA
    tree_data->node = NULL;
    tree_data->max_width_needed = 0;
    tree_data->max_height_needed = 0;
    tree_data->parent_cnt = 0;
    tree_data->scale = 1;
    tree_data->internal_padd = 3;
    tree_data->arrow_length = 20;
    tree_data->gap = 100;

    prepare_drawing_tree(root, NULL, tree_data, IMG_WIDTH / 2, 0, false);
    printf("gap: %d\n", tree_data->gap);
    tree_data->max_height_needed =
        tree_data->parent_cnt * BITMAP_SIZE * tree_data->scale + ((tree_data->internal_padd * 2) * tree_data->parent_cnt) - tree_data->parent_cnt + tree_data->gap * (tree_data->parent_cnt - 1);

    //  resize arrow acording to gap
    draw_tree(img, tree_data->node, *tree_data, tree_data->node->draw_x, tree_data->node->draw_y);
}

int main()
{
    const char *start_file = "C:\\Users\\marco\\Programming\\DirectoryTree";

    Node *root = NULL;
    TreeData *tree_data = (TreeData *)malloc(sizeof(TreeData));
    if (tree_data == NULL)
    {
        fprintf(stderr, "ERRROR: NOT ENOUGHT MEMORY FOR TreeData");
        return 1;
    }

    // save first parent
    if (root == NULL)
    {
        char name[512];
        const char *base = basename(start_file);
        strncpy(name, base, sizeof(name) - 1);
        name[sizeof(name) - 1] = '\0';
        root = new_node(name, PARENT);
    }

    tranverse(start_file, root);

    int w = IMG_WIDTH;
    int h = IMG_HEIGHT;

    unsigned char *img = malloc(IMG_WIDTH * IMG_HEIGHT * 3);
    if (img == NULL)
    {
        fprintf(stderr, "ERROR: OUT OF MEMORY FOR IMG");
        return 1;
    }

    load_tree(root, tree_data, img);

    printf("\n\nTREE\n\n");
    printf("tree_data: %i\n", tree_data->parent_cnt);
    // walk(root, 0, false);
    // walk_draw(tree_data->node, 0, true);
    // walk_draw_verbose(tree_data->node, 0, true);
    int ok = stbi_write_png("C:\\Users\\marco\\Programming\\DirectoryTree\\tree.png", w, h, 3, img, w * 3);
    if (!ok)
    {
        fprintf(stderr, "ERROR: FAILED TO WRITE PNG\n");
        return 1;
    }

    free(img);
    free_draw_tree(tree_data->node);
    free_node(root);
    free(tree_data);

    return 0;
}
