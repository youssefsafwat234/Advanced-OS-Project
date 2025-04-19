#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_NAME 10

typedef struct Block {
    int start;
    int end;
    char name[MAX_NAME]; // "Unused" or process name
    struct Block* next;
} Block;

Block* memory = NULL;
int total_memory = 0;

void insert_block_sorted(Block* new_block) {
    if (!memory || new_block->start < memory->start) {
        new_block->next = memory;
        memory = new_block;
        return;
    }
    Block* prev = memory;
    while (prev->next && prev->next->start < new_block->start)
        prev = prev->next;
    new_block->next = prev->next;
    prev->next = new_block;
}

void merge_unused_blocks() {
    Block* curr = memory;
    while (curr && curr->next) {
        if (strcmp(curr->name, "Unused") == 0 && strcmp(curr->next->name, "Unused") == 0) {
            curr->end = curr->next->end;
            Block* temp = curr->next;
            curr->next = temp->next;
            free(temp);
        }
        else {
            curr = curr->next;
        }
    }
}

void status_report() {
    Block* curr = memory;
    while (curr) {
        printf("Addresses [%d:%d] %s\n", curr->start, curr->end, curr->name);
        curr = curr->next;
    }
}

void release_block(char* process_name) {
    Block* curr = memory;
    while (curr) {
        if (strcmp(curr->name, process_name) == 0) {
            strcpy(curr->name, "Unused");
            merge_unused_blocks();
            return;
        }
        curr = curr->next;
    }
    printf("Process %s not found.\n", process_name);
}

void compact_memory() {
    Block* curr = memory;
    int next_start = 0;
    Block* new_memory = NULL;
    Block* new_memory_tail = NULL;  // To track the last block of the new memory list

    while (curr) {
        if (strcmp(curr->name, "Unused") != 0) {
            Block* block = malloc(sizeof(Block));
            strcpy(block->name, curr->name);
            block->start = next_start;
            block->end = next_start + (curr->end - curr->start);
            next_start = block->end + 1;
            block->next = NULL;

            // Insert this block into the new memory list
            if (!new_memory) {
                new_memory = block;
                new_memory_tail = new_memory;
            }
            else {
                new_memory_tail->next = block;
                new_memory_tail = block;
            }
        }
        curr = curr->next;
    }

    // Create a hole at the end of the compacted memory
    Block* hole = malloc(sizeof(Block));
    hole->start = next_start;
    hole->end = total_memory - 1;
    strcpy(hole->name, "Unused");
    hole->next = NULL;

    // Link the hole to the end of the compacted memory list
    if (!new_memory) {
        new_memory = hole;
    }
    else {
        new_memory_tail->next = hole;
    }

    // Update memory to point to the new compacted memory
    memory = new_memory;
}

void request_memory(char* name, int size, char strategy) {
    Block* best = NULL, * prev = NULL, * prev_best = NULL;
    Block* curr = memory, * prev_curr = NULL;

    while (curr) {
        if (strcmp(curr->name, "Unused") == 0 && (curr->end - curr->start + 1) >= size) {
            if (!best ||
                (strategy == 'B' && (curr->end - curr->start) < (best->end - best->start)) ||
                (strategy == 'W' && (curr->end - curr->start) > (best->end - best->start)) ||
                (strategy == 'F')) {
                best = curr;
                prev_best = prev_curr;
                if (strategy == 'F') break;
            }
        }
        prev_curr = curr;
        curr = curr->next;
    }

    if (!best) {
        printf("Not enough memory for process %s.\n", name);
        return;
    }

    int start = best->start;
    int end = start + size - 1;
    Block* new_block = malloc(sizeof(Block));
    strcpy(new_block->name, name);
    new_block->start = start;
    new_block->end = end;
    new_block->next = NULL;

    insert_block_sorted(new_block);

    if (end < best->end) {
        best->start = end + 1;
    }
    else {
        if (prev_best)
            prev_best->next = best->next;
        else
            memory = best->next;
        free(best);
    }
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        printf("Usage: ./allocator <memory size>\n");
        return 1;
    }

    total_memory = atoi(argv[1]);
    memory = malloc(sizeof(Block));
    memory->start = 0;
    memory->end = total_memory - 1;
    strcpy(memory->name, "Unused");
    memory->next = NULL;

    char command[100];
    char pname[MAX_NAME], strategy;
    int size;

    while (1) {
        printf("allocator> ");
        fgets(command, sizeof(command), stdin);

        if (strncmp(command, "RQ", 2) == 0) {
            sscanf(command, "RQ %s %d %c", pname, &size, &strategy);
            request_memory(pname, size, strategy);
        }
        else if (strncmp(command, "RL", 2) == 0) {
            sscanf(command, "RL %s", pname);
            release_block(pname);
        }
        else if (strncmp(command, "C", 1) == 0) {
            compact_memory();
        }
        else if (strncmp(command, "STAT", 4) == 0) {
            status_report();
        }
        else if (strncmp(command, "X", 1) == 0) {
            break;
        }
        else {
            printf("Invalid command.\n");
        }
    }

    // Free memory list
    while (memory) {
        Block* tmp = memory;
        memory = memory->next;
        free(tmp);
    }

    return 0;
}
