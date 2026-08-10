#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "memory_manager.h"

Heap heap = { NULL };
Graph proc_graph;
int pid_counter = 1;

#define HEAP_FILE "/Users/sakshibisht/Documents/project/heap_state.dat"
#define GRAPH_FILE "/Users/sakshibisht/Documents/project/graph_state.dat"

/* ------------------ HASH TABLE FOR PID LOOKUP ------------------ */

HashNode *pid_table[HASH_SIZE] = { NULL };

static int hash_pid(int pid) {
    if (pid < 0) pid = -pid;
    return pid % HASH_SIZE;
}

void pid_table_insert(int pid, Block *block) {
    int index = hash_pid(pid);
    HashNode *new_node = malloc(sizeof(HashNode));
    if (!new_node) return;
    new_node->pid = pid;
    new_node->block = block;
    new_node->next = pid_table[index];
    pid_table[index] = new_node;
}

Block *pid_table_lookup(int pid) {
    int index = hash_pid(pid);
    HashNode *curr = pid_table[index];
    while (curr) {
        if (curr->pid == pid)
            return curr->block;
        curr = curr->next;
    }
    return NULL;
}

void pid_table_delete(int pid) {
    int index = hash_pid(pid);
    HashNode *curr = pid_table[index];
    HashNode *prev = NULL;

    while (curr) {
        if (curr->pid == pid) {
            if (prev)
                prev->next = curr->next;
            else
                pid_table[index] = curr->next;
            free(curr);
            return;
        }
        prev = curr;
        curr = curr->next;
    }
}

void pid_table_clear(void) {
    for (int i = 0; i < HASH_SIZE; i++) {
        HashNode *curr = pid_table[i];
        while (curr) {
            HashNode *temp = curr;
            curr = curr->next;
            free(temp);
        }
        pid_table[i] = NULL;
    }
}

/* ------------------ BST FOR FREE BLOCKS (by size) ------------------ */

BSTNode *free_bst_root = NULL;

static int bst_compare_blocks(Block *a, Block *b) {
    if (a->size < b->size) return -1;
    if (a->size > b->size) return 1;
    if (a->start < b->start) return -1;
    if (a->start > b->start) return 1;
    return 0;
}

static BSTNode *bst_create_node(Block *block) {
    BSTNode *n = malloc(sizeof(BSTNode));
    if (!n) return NULL;
    n->block = block;
    n->left = n->right = NULL;
    return n;
}

static BSTNode *bst_insert_node(BSTNode *root, Block *block) {
    if (!root) return bst_create_node(block);
    int cmp = bst_compare_blocks(block, root->block);
    if (cmp < 0)
        root->left = bst_insert_node(root->left, block);
    else
        root->right = bst_insert_node(root->right, block);
    return root;
}

void bst_insert(Block *block) {
    if (!block || block->allocated) return;
    free_bst_root = bst_insert_node(free_bst_root, block);
}

static BSTNode *bst_min_node(BSTNode *root) {
    while (root && root->left)
        root = root->left;
    return root;
}

static BSTNode *bst_delete_by_key(BSTNode *root, size_t size, size_t start) {
    if (!root) return NULL;
    Block key = {.size = size, .start = start};
    int cmp = bst_compare_blocks(&key, root->block);

    if (cmp < 0)
        root->left = bst_delete_by_key(root->left, size, start);
    else if (cmp > 0)
        root->right = bst_delete_by_key(root->right, size, start);
    else {
        if (!root->left) {
            BSTNode *r = root->right;
            free(root);
            return r;
        } else if (!root->right) {
            BSTNode *l = root->left;
            free(root);
            return l;
        } else {
            BSTNode *min = bst_min_node(root->right);
            root->block = min->block;
            root->right = bst_delete_by_key(root->right, min->block->size, min->block->start);
        }
    }
    return root;
}

void bst_delete(Block *block) {
    if (!block) return;
    free_bst_root = bst_delete_by_key(free_bst_root, block->size, block->start);
}

static void bst_clear_node(BSTNode *root) {
    if (!root) return;
    bst_clear_node(root->left);
    bst_clear_node(root->right);
    free(root);
}

void bst_clear(void) {
    bst_clear_node(free_bst_root);
    free_bst_root = NULL;
}

void bst_rebuild_from_heap(void) {
    bst_clear();
    Block *c = heap.head;
    while (c) {
        if (!c->allocated)
            bst_insert(c);
        c = c->next;
    }
}

Block *bst_find_best_fit(size_t requested_size) {
    BSTNode *curr = free_bst_root;
    BSTNode *best = NULL;
    while (curr) {
        if (curr->block->size == requested_size) {
            best = curr;
            break;
        } else if (curr->block->size > requested_size) {
            best = curr;
            curr = curr->left;
        } else {
            curr = curr->right;
        }
    }
    return best ? best->block : NULL;
}

/* ------------------ HEAP FUNCTIONS ------------------ */

void heap_init(void) {
    Block *c = heap.head;
    while (c) {
        Block *tmp = c->next;
        free(c);
        c = tmp;
    }

    heap.head = malloc(sizeof(Block));
    if (!heap.head) return;

    heap.head->pid = -1;
    heap.head->allocated = 0;
    heap.head->size = TOTAL_MEMORY;
    heap.head->start = 0;
    heap.head->end = TOTAL_MEMORY;
    heap.head->next = NULL;

    bst_clear();
    bst_insert(heap.head);
}

int memory_allocate_auto(size_t size) {
    if (size == 0 || size > TOTAL_MEMORY)
        return -1;

    Block *chosen = bst_find_best_fit(size);

    if (!chosen) {
        Block *curr = heap.head;
        while (curr) {
            if (!curr->allocated && curr->size >= size) {
                chosen = curr;
                break;
            }
            curr = curr->next;
        }
    }

    if (!chosen) return -1;

    bst_delete(chosen);

    size_t original_end = chosen->end;

    if (chosen->size > size) {
        Block *newBlock = malloc(sizeof(Block));
        if (!newBlock) {
            bst_insert(chosen);
            return -1;
        }
        newBlock->pid = -1;
        newBlock->allocated = 0;
        newBlock->size = chosen->size - size;
        newBlock->start = chosen->start + size;
        newBlock->end = original_end;
        newBlock->next = chosen->next;
        chosen->next = newBlock;
        bst_insert(newBlock);
    }

    chosen->allocated = 1;
    chosen->pid = pid_counter++;
    chosen->size = size;
    chosen->end = chosen->start + size;

    pid_table_insert(chosen->pid, chosen);
    return chosen->pid;
}

int memory_free_pid(int pid) {
    Block *block = pid_table_lookup(pid);
    if (!block || !block->allocated)
        return -1;

    block->allocated = 0;
    block->pid = -1;

    bst_insert(block);

    Block *curr = heap.head;
    Block *prev = NULL;

    while (curr) {
        if (curr == block) {
            if (curr->next && !curr->next->allocated) {
                Block *right = curr->next;
                bst_delete(right);
                curr->size += right->size;
                curr->end = right->end;
                curr->next = right->next;
                free(right);
            }
            if (prev && !prev->allocated) {
                bst_delete(prev);
                bst_delete(curr);
                prev->size += curr->size;
                prev->end = curr->end;
                prev->next = curr->next;
                free(curr);
                bst_insert(prev);
            } else {
                bst_delete(block);
                bst_insert(prev && !prev->allocated ? prev : block);
            }
            break;
        }
        prev = curr;
        curr = curr->next;
    }

    pid_table_delete(pid);
    return 0;
}

void memory_compact(void) {
    Block *curr = heap.head;
    size_t next_start = 0;
    Block *newHead = NULL;
    Block *last = NULL;

    while (curr) {
        if (curr->allocated) {
            curr->start = next_start;
            curr->end = curr->start + curr->size;
            next_start = curr->end;

            if (!newHead)
                newHead = curr;
            else
                last->next = curr;
            last = curr;
        }
        curr = curr->next;
    }

    size_t free_space = TOTAL_MEMORY - next_start;
    Block *freeBlock = malloc(sizeof(Block));
    if (!freeBlock) return;

    freeBlock->pid = -1;
    freeBlock->allocated = 0;
    freeBlock->size = free_space;
    freeBlock->start = next_start;
    freeBlock->end = next_start + free_space;
    freeBlock->next = NULL;

    if (last)
        last->next = freeBlock;
    else
        newHead = freeBlock;

    heap.head = newHead;
    bst_rebuild_from_heap();
}

void display_memory(void) {
    Block *curr = heap.head;

    printf("-------------------------------------------------------\n");
    printf("%-8s%-10s%-10s%-12s%-10s\n", "PID", "START", "END", "SIZE(KB)", "STATUS");
    printf("-------------------------------------------------------\n");

    while (curr) {
        printf("%-8d%-10zu%-10zu%-12zu%-10s\n",
               curr->pid,
               curr->start,
               curr->end,
               curr->size,
               curr->allocated ? "Used" : "Free");
        curr = curr->next;
    }

    printf("-------------------------------------------------------\n");
}

/* ------------------ SAVE / LOAD HEAP ------------------ */

void save_heap(const char *filename) {
    FILE *fp = fopen(filename, "wb");
    if (!fp) return;

    Block *curr = heap.head;
    while (curr) {
        fwrite(curr, sizeof(Block), 1, fp);
        curr = curr->next;
    }

    fclose(fp);
}

void load_heap(const char *filename) {
    FILE *fp = fopen(filename, "rb");
    if (!fp) {
        heap_init();
        return;
    }

    Block *cptr = heap.head;
    while (cptr) {
        Block *tmp = cptr->next;
        free(cptr);
        cptr = tmp;
    }

    heap.head = NULL;
    Block *prev = NULL;
    Block temp;

    pid_table_clear();
    bst_clear();

    while (fread(&temp, sizeof(Block), 1, fp) == 1) {
        Block *newBlock = malloc(sizeof(Block));
        if (!newBlock) {
            fclose(fp);
            heap_init();
            return;
        }
        *newBlock = temp;
        newBlock->next = NULL;

        if (!heap.head)
            heap.head = newBlock;
        else
            prev->next = newBlock;
        prev = newBlock;

        if (newBlock->allocated)
            pid_table_insert(newBlock->pid, newBlock);
        else
            bst_insert(newBlock);
    }

    fclose(fp);

    pid_counter = 1;
    Block *c = heap.head;
    while (c) {
        if (c->allocated && c->pid >= pid_counter)
            pid_counter = c->pid + 1;
        c = c->next;
    }
}

/* ------------------ GRAPH FUNCTIONS ------------------ */

void graph_init(Graph *g) {
    for (int i = 0; i < MAX_PROCESSES; i++)
        g->adjList[i] = NULL;
}

void graph_add_edge(Graph *g, int from, int to) {
    if (from < 0 || from >= MAX_PROCESSES || to < 0 || to >= MAX_PROCESSES)
        return;

    Node *newNode = malloc(sizeof(Node));
    if (!newNode) return;
    newNode->pid = to;
    newNode->next = g->adjList[from];
    g->adjList[from] = newNode;
}

void graph_display(Graph *g) {
    printf("\n--- Process Graph ---\n");
    for (int i = 0; i < MAX_PROCESSES; i++) {
        if (g->adjList[i]) {
            printf("PID %d -> ", i);
            Node *curr = g->adjList[i];
            while (curr) {
                printf("%d ", curr->pid);
                curr = curr->next;
            }
            printf("\n");
        }
    }
}

void save_graph(const char *filename) {
    FILE *fp = fopen(filename, "wb");
    if (!fp) return;

    for (int i = 0; i < MAX_PROCESSES; i++) {
        Node *temp = proc_graph.adjList[i];
        while (temp) {
            fwrite(&i, sizeof(int), 1, fp);
            fwrite(&temp->pid, sizeof(int), 1, fp);
            temp = temp->next;
        }
    }

    fclose(fp);
}

void load_graph(const char *filename) {
    FILE *fp = fopen(filename, "rb");
    if (!fp) {
        graph_init(&proc_graph);
        return;
    }

    graph_init(&proc_graph);
    int from, to;
    while (fread(&from, sizeof(int), 1, fp) == 1) {
        if (fread(&to, sizeof(int), 1, fp) != 1)
            break;
        graph_add_edge(&proc_graph, from, to);
    }

    fclose(fp);
}

/* ------------------ SYSTEM RESET ------------------ */

void reset_system(void) {
    Block *c = heap.head;
    while (c) {
        Block *tmp = c->next;
        free(c);
        c = tmp;
    }
    heap.head = NULL;

    heap_init();
    graph_init(&proc_graph);
    pid_counter = 1;

    pid_table_clear();
    bst_clear();

    FILE *fp = fopen(HEAP_FILE, "wb");
    if (fp) fclose(fp);
    fp = fopen(GRAPH_FILE, "wb");
    if (fp) fclose(fp);
}
