#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "memory_manager.h"

#define HEAP_FILE "/Users/sakshibisht/Documents/project/heap_state.dat"
#define GRAPH_FILE "/Users/sakshibisht/Documents/project/graph_state.dat"

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Usage: ./backend_main <command> [args]\n");
        return 1;
    }

    load_heap(HEAP_FILE);
    load_graph(GRAPH_FILE);

    if (!heap.head) {
        heap_init();
    }

    if (strcmp(argv[1], "allocate") == 0 && argc == 3) {
        size_t size = atoi(argv[2]);
        int pid = memory_allocate_auto(size);
        if (pid != -1)
            printf("✅ Allocated %zuKB to PID %d\n", size, pid);
        else
            printf("❌ Allocation failed\n");
        save_heap(HEAP_FILE);
        save_graph(GRAPH_FILE);
        display_memory();
    } 
    else if (strcmp(argv[1], "deallocate") == 0 && argc == 3) {
        int pid = atoi(argv[2]);
        if (memory_free_pid(pid) == -1)
            printf("⚠ PID %d not found\n", pid);
        else
            printf("🗑 Deallocated PID %d\n", pid);
        save_heap(HEAP_FILE);
        save_graph(GRAPH_FILE);
        display_memory();
    }
    else if (strcmp(argv[1], "compact") == 0) {
        memory_compact();
        printf("🧹 Memory compacted successfully.\n");
        save_heap(HEAP_FILE);
        save_graph(GRAPH_FILE);
        display_memory();
    }
    else if (strcmp(argv[1], "display") == 0) {
        display_memory();
    }
    else if (strcmp(argv[1], "add_relation") == 0 && argc == 4) {
        int from = atoi(argv[2]);
        int to = atoi(argv[3]);
        graph_add_edge(&proc_graph, from, to);
        save_graph(GRAPH_FILE);
        printf("🔗 Added relation %d -> %d\n", from, to);
    }
    else if (strcmp(argv[1], "show_graph") == 0) {
        graph_display(&proc_graph);
    }
    else if (strcmp(argv[1], "logout") == 0) {
        reset_system();
        printf("🔒 Session cleared.\n");
        save_heap(HEAP_FILE);
        save_graph(GRAPH_FILE);
    }
    else {
        printf("❌ Invalid command.\n");
    }

    return 0;
}
