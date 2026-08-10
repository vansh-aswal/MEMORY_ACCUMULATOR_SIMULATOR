#ifndef MEMORY_MANAGER_H
#define MEMORY_MANAGER_H

#include <stddef.h>

/* ------------------ CONFIGURABLE LIMITS ------------------ */
#define TOTAL_MEMORY 1024        /* Total simulated memory size (KB) */
#define MAX_PROCESSES 100        /* Maximum number of processes */
#define HASH_SIZE 100            /* Hash table bucket count */

/* ------------------ CORE STRUCTURE DEFINITIONS ------------------ */

/* Represents a block in the simulated heap */
typedef struct Block {
    int pid;                     /* Process ID (-1 if free) */
    size_t size;                 /* Size of block (in KB) */
    size_t start;                /* Start address */
    size_t end;                  /* End address (start + size) */
    int allocated;               /* 1 = allocated, 0 = free */
    struct Block *next;          /* Pointer to next block in linked list */
} Block;

/* Represents the heap (linked list of blocks) */
typedef struct {
    Block *head;
} Heap;

/* Graph node for process relations */
typedef struct Node {
    int pid;
    struct Node *next;
} Node;

/* Graph using adjacency list */
typedef struct {
    Node *adjList[MAX_PROCESSES];
} Graph;

/* ------------------ GLOBAL VARIABLES ------------------ */

extern Heap heap;
extern Graph proc_graph;
extern int pid_counter;

/* ------------------ PUBLIC HEAP FUNCTIONS ------------------ */

/**
 * Initialize the heap with a single large free block.
 */
void heap_init(void);

/**
 * Automatically allocate a block using Best Fit (or First Fit if unavailable).
 * Returns PID of allocated block, or -1 if allocation fails.
 */
int memory_allocate_auto(size_t size);

/**
 * Free a block associated with the given PID.
 * Returns 0 on success, -1 if PID not found.
 */
int memory_free_pid(int pid);

/**
 * Compact memory (move allocated blocks to start and merge free space).
 */
void memory_compact(void);

/**
 * Display heap layout in tabular format.
 */
void display_memory(void);

/* ------------------ GRAPH FUNCTIONS ------------------ */

/**
 * Initialize process graph (clears adjacency list).
 */
void graph_init(Graph *g);

/**
 * Add directed edge: process `from` depends on process `to`.
 */
void graph_add_edge(Graph *g, int from, int to);

/**
 * Display process dependency graph.
 */
void graph_display(Graph *g);

/* ------------------ PERSISTENCE ------------------ */

/**
 * Save and load heap state to/from a binary file.
 */
void save_heap(const char *filename);
void load_heap(const char *filename);

/**
 * Save and load process graph to/from a binary file.
 */
void save_graph(const char *filename);
void load_graph(const char *filename);

/* ------------------ SYSTEM RESET ------------------ */

/**
 * Reset the entire system: heap, graph, PID counter, hash table, BST.
 * Also truncates persistence files.
 */
void reset_system(void);

/* ================================================================
 * ============= INTERNAL DATA STRUCTURE INTERFACES ===============
 * ================================================================
 * These are safe to expose in developer mode for visualization,
 * debugging, and unit testing.
 */

/* ------------------ HASH TABLE (PID → Block*) ------------------ */

/* Node for hash table chain */
typedef struct HashNode {
    int pid;
    Block *block;
    struct HashNode *next;
} HashNode;

/* Global hash table (declared in memory_manager.c) */
extern HashNode *pid_table[HASH_SIZE];

/**
 * Insert mapping PID → Block* into hash table.
 */
void pid_table_insert(int pid, Block *block);

/**
 * Lookup Block* by PID (returns NULL if not found).
 */
Block *pid_table_lookup(int pid);

/**
 * Delete mapping for given PID.
 */
void pid_table_delete(int pid);

/**
 * Clear the entire PID table (free all nodes).
 */
void pid_table_clear(void);

/**
 * Display current contents of hash table (for debugging).
 */
void display_pid_table(void);

/* ------------------ BST (Free Block Index by Size) ------------------ */

/* Node structure for BST (each node points to a free Block) */
typedef struct BSTNode {
    Block *block;
    struct BSTNode *left;
    struct BSTNode *right;
} BSTNode;

/* Global BST root (declared in memory_manager.c) */
extern BSTNode *free_bst_root;

/**
 * Insert a free block into the BST.
 */
void bst_insert(Block *block);

/**
 * Delete a block (by size and start address) from the BST.
 */
void bst_delete(Block *block);

/**
 * Clear entire BST (free all nodes).
 */
void bst_clear(void);

/**
 * Rebuild BST from the current heap (scans all free blocks).
 */
void bst_rebuild_from_heap(void);

/**
 * Display BST in inorder traversal (sorted by block size, start address).
 */
void bst_display(void);

/**
 * Find the best-fit free block (smallest block with size >= requested_size).
 * Returns pointer to Block, or NULL if none found.
 */
Block *bst_find_best_fit(size_t requested_size);

#endif /* MEMORY_MANAGER_H */
