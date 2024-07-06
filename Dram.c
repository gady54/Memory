#include <stdio.h>
#include <stdlib.h>
#include <stdint.h> // for uint32_t

#define BUS_WIDTH 4
#define CHANNELS 1
#define DIMMS 1
#define BANKS 4
#define RAS_TIME 100
#define CAS_TIME 50
#define CACHE_BLOCK_SIZE 32 // Assuming 32 bytes cache block

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

// Function to initialize the DRAM structure
void init_dram(DRAM *dram) {
    for (int i = 0; i < BANKS; i++) {
        dram->banks[i].active_row = -1;         // Set all banks' active rows to -1 (no row is active)
        dram->banks[i].time_last_accessed = -1; // Set all banks' last accessed time to -1
    }
    dram->time = 0; // Initialize the global time counter to 0
}

// Function for row interleaving: translate an address to bank, row, and column
void row_interleaving(uint32_t address, int *bank, int *row, int *col) {
    *col = (int)(address & 0x3);       // Column is determined by the least significant 2 bits
    *bank = (int)((address >> 2) & 0x3); // Bank is determined by the next 2 bits
    *row = (int)(address >> 4);        // Row is determined by the remaining bits
}

// Function for cache block interleaving: translate an address to bank, row, and column
void cache_block_interleaving(uint32_t address, int *bank, int *row, int *col) {
    *col = (int)(address & 0x3); // Column is determined by the least significant 2 bits
    *bank = (int)((address / CACHE_BLOCK_SIZE) % BANKS); // Bank is determined by the cache block size
    *row = (int)(address / (CACHE_BLOCK_SIZE * BANKS)); // Row is determined by dividing by cache block size and number of banks
}

// Function to access a specific address in DRAM
int access_dram(DRAM *dram, uint32_t address, void (*address_mapping)(uint32_t, int*, int*, int*)) {
    int bank, row, col;
    int latency = 0;
    
    // Translate the address using the provided mapping function
    address_mapping(address, &bank, &row, &col);
    
    // Access the specified bank, row, and column
    DRAMBank *current_bank = &dram->banks[bank];

    // Check if the requested row is already active
    if (current_bank->active_row != row) {
        // If not, add RAS latency and activate the requested row
        latency += RAS_TIME;
        current_bank->active_row = row;
    }

    // Add CAS latency for accessing the column
    latency += CAS_TIME;

    // Update the global time and the bank's last accessed time
    dram->time += latency;
    current_bank->time_last_accessed = dram->time;

    return latency; // Return the total latency for this access
}

// Function to print the state of the DRAM banks
void print_dram_state(DRAM *dram) {
    printf("DRAM State:\n");
    for (int i = 0; i < BANKS; i++) {
        printf("Bank %d: Active Row = %d, Last Accessed Time = %d\n",
               i, dram->banks[i].active_row, dram->banks[i].time_last_accessed);
    }
    printf("Global Time: %d\n", dram->time);
}

// Function to visualize the DRAM access for multiple addresses
void visualize_dram_access(DRAM *dram, uint32_t addresses[], int num_addresses, void (*address_mapping)(uint32_t, int*, int*, int*)) {
    for (int i = 0; i < num_addresses; i++) {
        printf("\nAccessing DRAM address 0x%x\n", addresses[i]);
        int latency = access_dram(dram, addresses[i], address_mapping);
        printf("Latency: %d cycles\n", latency);
        print_dram_state(dram);
    }
}

// Main function to demonstrate the DRAM access simulation
int main() {
    DRAM dram; // Declare a DRAM structure

    uint32_t addresses[] = {0x0000, 0x0010, 0x0040, 0x0040, 0x0cd5, 0xff02, 0xf5ca, 0xf8a03, 0x81ff};
    int num_addresses = sizeof(addresses) / sizeof(addresses[0]);

    // Access various addresses using row interleaving
    printf("Using Row Interleaving:\n");
    init_dram(&dram); // Initialize the DRAM
    visualize_dram_access(&dram, addresses, num_addresses, row_interleaving);

    // Access various addresses using cache block interleaving
    printf("\nUsing Cache Block Interleaving:\n");
    init_dram(&dram); // Re-initialize the DRAM
    visualize_dram_access(&dram, addresses, num_addresses, cache_block_interleaving);

    return 0;
}
