#include <stdio.h>
#include <stdlib.h>
#include <stdint.h> // for uint32_t
#include <stdbool.h>

#define BUS_WIDTH 4
#define CHANNELS 1
#define DIMMS 1
#define BANKS 4
#define ROWS 262144 // 2^18 rows
#define COLUMNS 1024 // 2^10 columns
#define RAS_TIME 100
#define CAS_TIME 50
#define RANK 1
#define CACHE_BLOCK_SIZE 64 // Assuming 64 bytes cache block
#define PRECHARGE_TIME 50 // Hypothetical precharge time

// Global variable to store the last accessed address
uint32_t last_accessed_address = 0;

// Structure representing a DRAM bank
typedef struct {
    int active_row;         // Currently active row in the bank (-1 if no row is active)
    int time_last_accessed; // Time when the bank was last accessed
} DRAMBank;

// Structure representing the entire DRAM
typedef struct {
    DRAMBank banks[BANKS];  // Array of banks in the DRAM
    int time;               // Global time counter
} DRAM;

// Structure representing a memory request
typedef struct {
    uint32_t address;
    int time_arrived;
} MemoryRequest;

#define MAX_REQUESTS 32
MemoryRequest request_queue[MAX_REQUESTS];
int queue_size = 0;

// Function for row interleaving: translate an address to bank, row, and column
void row_interleaving(uint32_t address, int *bank, int *row, int *col) {
    *col = (int)((address >> 2) & 0x3FF);        // Column is determined by bits 2-11 (10 bits)
    *bank = (int)((address >> 12) & 0x3);        // Bank is determined by bits 12-13 (2 bits)
    *row = (int)((address >> 14) & 0x3FFFF);     // Row is determined by bits 14-31 (18 bits)
}

// Function for cache block interleaving: translate an address to bank, row, and column
void cache_block_interleaving(uint32_t address, int *bank, int *row, int *col) {
    int cache_block_offset_bits = 6; // log2(64), where 64 is the CACHE_BLOCK_SIZE
    *bank = (address >> cache_block_offset_bits) & 0x3; // Bank is determined by bits 6-7 (2 bits)
    *col = (address & 0x3F) | ((address >> 8) & 0x300); // Column determined by low 6 bits and 2 bits after 8 bits
    *row = (address >> 10) & 0x3FFFF; // Row is determined by bits 10-31 (22 bits)
}

// Hypothetical function to simulate sending the address to the cache
void send_to_cache(uint32_t address) {
    printf("Address 0x%08x is sent to the cache.\n", address);
}

// Function to access a specific address in DRAM through the controller
uint32_t access_dram(DRAM *dram, uint32_t address, int *total_latency, void (*address_mapping)(uint32_t, int*, int*, int*), bool is_cache_block_interleaving) {
    int bank, row, col;
    int latency = 0;

    // Translate the address using the provided mapping function
    address_mapping(address, &bank, &row, &col);

    // Access the specified bank, row, and column
    DRAMBank *current_bank = &dram->banks[bank];

    // Store the previously active row
    int previous_active_row = current_bank->active_row;

    if (is_cache_block_interleaving) {
        // Cache block interleaving logic
        if (current_bank->active_row != row) {
            // If there was a previously active row, add precharge latency
            if (previous_active_row != -1) {
                latency += PRECHARGE_TIME;
            }
            // Add RAS latency to activate the new row
            latency += RAS_TIME;
            current_bank->active_row = row;
        }
    } else {
        // Row interleaving logic
        if (current_bank->active_row != row) {
            // If there was a previously active row, add precharge latency
            if (previous_active_row != -1) {
                latency += PRECHARGE_TIME;
            }
            // Add RAS latency to activate the new row
            latency += RAS_TIME;
            current_bank->active_row = row;
        }
    }

    // Add CAS latency for accessing the column
    latency += CAS_TIME;

    // Send the address to the cache
    send_to_cache(address);

    // Update the global time and the bank's last accessed time
    dram->time += latency;
    current_bank->time_last_accessed = dram->time;

    // Update the last accessed address
    last_accessed_address = address;

    printf("%-4d | %-4d | %-6d | 0x%08x | %-9s | %-7d cycles\n",
           bank, row, col, address, (previous_active_row == row) ? "YES" : "NO", latency);

    *total_latency = latency;
    return address; // Return the accessed address
}

// Function to print the state of the DRAM banks
void print_dram_state(DRAM *dram) {
    printf("DRAM State:\n");
    printf("-------------------------------------------------------------\n");
    printf("Bank | Active Row | Last Accessed Time\n");
    printf("-------------------------------------------------------------\n");
    for (int i = 0; i < BANKS; i++) {
        printf("%-4d | %-10d | %-18d\n", i, dram->banks[i].active_row, dram->banks[i].time_last_accessed);
    }
    printf("-------------------------------------------------------------\n");
    printf("Global Time: %d\n", dram->time);
}

// Function to visualize the DRAM access for multiple addresses
void visualize_dram_access(DRAM *dram, uint32_t addresses[], int num_addresses, void (*address_mapping)(uint32_t, int*, int*, int*), bool is_cache_block_interleaving) {
    int total_latency;
    int total_time_access = 0;
    printf("Bank | Row  | Column | Address     | Row Active | Latency\n");
    printf("-------------------------------------------------------------\n");
    for (int i = 0; i < num_addresses; i++) {
        access_dram(dram, addresses[i], &total_latency, address_mapping, is_cache_block_interleaving);
        total_time_access += total_latency;
    }
    printf("-------------------------------------------------------------\n");
    print_dram_state(dram);
    printf("Total Time Access: %d cycles\n", total_time_access);
}

// Function to add a memory request to the queue
void add_request(uint32_t address, int time_arrived) {
    if (queue_size < MAX_REQUESTS) {
        request_queue[queue_size].address = address;
        request_queue[queue_size].time_arrived = time_arrived;
        queue_size++;
    } else {
        printf("Request queue full!\n");
    }
}

// Function to get the next request index based on the FCFS policy
int get_next_request_index() {
    if (queue_size == 0) return -1;
    int oldest_index = 0;
    for (int i = 1; i < queue_size; i++) {
        if (request_queue[i].time_arrived < request_queue[oldest_index].time_arrived) {
            oldest_index = i;
        }
    }
    return oldest_index;
}

// Function to read addresses from a file
int read_addresses_from_file(const char *filename, uint32_t **addresses) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        printf("Failed to open file %s\n", filename);
        return 0;
    }

    int count = 0;
    uint32_t address;
    while (fscanf(file, "%x", &address) != EOF) {
        count++;
    }

    rewind(file);
    *addresses = (uint32_t *)malloc((size_t)count * sizeof(uint32_t)); // Explicitly cast count to size_t
    if (!*addresses) {
        perror("Failed to allocate memory for addresses");
        fclose(file); // Ensure file is closed before exiting
        exit(EXIT_FAILURE);
    }
    for (int i = 0; i < count; i++) {
        fscanf(file, "%x", &(*addresses)[i]);
    }

    fclose(file);
    return count;
}

// Main function to demonstrate the DRAM access simulation
int main() {
    DRAM dram = {0}; // Declare and initialize a DRAM structure
    // Initialize DRAM banks
    for (int i = 0; i < BANKS; i++) {
        dram.banks[i].active_row = -1;
        dram.banks[i].time_last_accessed = 0;
    }

    uint32_t *addresses;
    int num_addresses = read_addresses_from_file("address.txt", &addresses);

    if (num_addresses == 0) {
        printf("No addresses to process. Exiting.\n");
        return 1;
    }

    int choice;
    void (*address_mapping)(uint32_t, int*, int*, int*);
    bool is_cache_block_interleaving = false;
    printf("Choose address mapping method:\n");
    printf("1. Row Interleaving\n");
    printf("2. Cache Block Interleaving\n");
    printf("Enter your choice: ");
    scanf("%d", &choice);

    // Set the address mapping function based on user choice
    if (choice == 1) {
        address_mapping = row_interleaving;
        printf("Using Row Interleaving:\n");
    } else if (choice == 2) {
        address_mapping = cache_block_interleaving;
        is_cache_block_interleaving = true;
        printf("Using Cache Block Interleaving:\n");
    } else {
        printf("Invalid choice. Exiting.\n");
        free(addresses);
        return 1;
    }

    // Add requests to the queue
    for (int i = 0; i < num_addresses; i++) {
        add_request(addresses[i], dram.time);
    }

    // Process requests using the FCFS policy
    while (queue_size > 0) {
        int next_request_index = get_next_request_index();
        if (next_request_index == -1) break;

        uint32_t address = request_queue[next_request_index].address;
        int total_latency;
        access_dram(&dram, address, &total_latency, address_mapping, is_cache_block_interleaving);

        // Remove the processed request from the queue
        for (int i = next_request_index; i < queue_size - 1; i++) {
            request_queue[i] = request_queue[i + 1];
        }
        queue_size--;
    }

    printf("Last accessed address: 0x%08x\n", last_accessed_address);

    // Free the allocated memory
    free(addresses);

    return 0;
}