#include <stdio.h>
#include <stdlib.h>
#include <stdint.h> // for uint8_t

#define BUS_WIDTH 4
#define CHANNELS 1
#define DIMMS 1
#define BANKS 4
#define ROWS 16
#define RAS_TIME 100
#define CAS_TIME 50
#define CACHE_BLOCK_SIZE 4 // Assuming 4 bytes cache block

// Global variable to store the last accessed address
uint8_t last_accessed_address = 0;

// Structure representing a DRAM bank
typedef struct {
    int active_row;         // Currently active row in the bank (-1 if no row is active)
    int time_last_accessed; // Time when the bank was last accessed
    uint8_t **stored_addresses; // Array to store addresses in the bank
    int stored_count;       // Number of stored addresses in the bank
    int row_counts[ROWS];   // Array to count the number of addresses per row
} DRAMBank;

// Structure representing the entire DRAM
typedef struct {
    DRAMBank banks[BANKS];  // Array of banks in the DRAM
    int time;               // Global time counter
} DRAM;

// Structure representing the DRAM Controller
typedef struct {
    DRAM *dram;             // Pointer to the DRAM
    void (*address_mapping)(uint8_t, int*, int*, int*); // Function pointer for address mapping
} DRAMController;

// Function to initialize the DRAM structure
void init_dram(DRAM *dram) {
    for (int i = 0; i < BANKS; i++) {
        dram->banks[i].active_row = -1;         // Set all banks' active rows to -1 (no row is active)
        dram->banks[i].time_last_accessed = -1; // Set all banks' last accessed time to -1
        dram->banks[i].stored_count = 0;        // Initialize stored addresses count to 0
        dram->banks[i].stored_addresses = (uint8_t **)malloc(ROWS * sizeof(uint8_t *));
        for (int j = 0; j < ROWS; j++) {
            dram->banks[i].stored_addresses[j] = (uint8_t *)malloc(16 * sizeof(uint8_t)); // Assume each row can store 16 addresses
            dram->banks[i].row_counts[j] = 0;   // Initialize row counts to 0
        }
    }
    dram->time = 0; // Initialize the global time counter to 0
}

// Function to free the allocated memory in the DRAM structure
void free_dram(DRAM *dram) {
    for (int i = 0; i < BANKS; i++) {
        for (int j = 0; j < ROWS; j++) {
            free(dram->banks[i].stored_addresses[j]);
        }
        free(dram->banks[i].stored_addresses);
    }
}

// Function to initialize the DRAM Controller
void init_dram_controller(DRAMController *controller, DRAM *dram, void (*address_mapping)(uint8_t, int*, int*, int*)) {
    controller->dram = dram;
    controller->address_mapping = address_mapping;
}

// Function for row interleaving: translate an address to bank, row, and column
void row_interleaving(uint8_t address, int *bank, int *row, int *col) {
    *col = (int)((address >> 0) & 0x03);        // Column is determined by bits 0-1 (2 bits)
    *bank = (int)((address >> 2) & 0x03);       // Bank is determined by bits 2-3 (2 bits)
    *row = (int)((address >> 4) & 0x0F);        // Row is determined by bits 4-7 (4 bits)
}

// Function for cache block interleaving: translate an address to bank, row, and column
void cache_block_interleaving(uint8_t address, int *bank, int *row, int *col) {
    *col = (int)(address & 0x03); // Column is determined by the least significant 2 bits
    *bank = (int)((address / CACHE_BLOCK_SIZE) % BANKS); // Bank is determined by the cache block size
    *row = (int)(address / (CACHE_BLOCK_SIZE * BANKS)); // Row is determined by dividing by cache block size and number of banks
}

// Function to check if an address is present in the bank
int is_address_in_bank(DRAMBank *bank, uint8_t address, int row) {
    for (int i = 0; i < bank->row_counts[row]; i++) {
        if (bank->stored_addresses[row][i] == address) {
            return 1;
        }
    }
    return 0;
}

// Function to save address in the bank
void save_address_in_bank(DRAMBank *bank, uint8_t address, int row) {
    if (bank->row_counts[row] < 16) { // Check to ensure there is space to store the address
        bank->stored_addresses[row][bank->row_counts[row]++] = address;
    } else {
        printf("Row %d in Bank is full, cannot store more addresses.\n", row);
    }
}

// Function to find a new bank or row if the current row is full
void find_new_bank_or_row(DRAM *dram, int *bank, int *row) {
    for (int b = 0; b < BANKS; b++) {
        for (int r = 0; r < ROWS; r++) {
            if (dram->banks[b].row_counts[r] < 16) {
                *bank = b;
                *row = r;
                return;
            }
        }
    }
    printf("All banks and rows are full.\n");
}

// Hypothetical function to simulate sending the address to the cache
void send_to_cache(uint8_t address) {
    printf("Address 0x%02x is sent to the cache.\n", address);
}

// Function to access a specific address in DRAM through the controller
uint8_t access_dram(DRAMController *controller, uint8_t address, int *total_latency) {
    int bank, row, col;
    int latency = 0;
    DRAM *dram = controller->dram;

    // Translate the address using the provided mapping function
    controller->address_mapping(address, &bank, &row, &col);
    
    // Access the specified bank, row, and column
    DRAMBank *current_bank = &dram->banks[bank];

    // Check if the requested address is already in the bank
    if (is_address_in_bank(current_bank, address, row)) {
        // Send the address to the cache if already present
        send_to_cache(address);
        latency += CAS_TIME; // Add CAS latency for accessing the column
    } else {
        // Check if the current row is full
        if (current_bank->row_counts[row] >= 16) {
            printf("Row %d in Bank %d is full. Finding new bank or row.\n", row, bank);
            find_new_bank_or_row(dram, &bank, &row);
            current_bank = &dram->banks[bank]; // Update the current bank reference
        }

        // Save the address in the bank if not present
        save_address_in_bank(current_bank, address, row);

        // Check if the requested row is already active
        if (current_bank->active_row != row) {
            // If not, add RAS latency and activate the requested row
            latency += RAS_TIME;
            current_bank->active_row = row;
        }
        
        // Add CAS latency for accessing the column
        latency += CAS_TIME;
    }

    // Update the global time and the bank's last accessed time
    dram->time += latency;
    current_bank->time_last_accessed = dram->time;

    // Update the last accessed address
    last_accessed_address = address;

    printf("%-4d | %-4d | %-6d | 0x%02x | %-9s | %-7d cycles\n",
           bank, row, col, address, current_bank->active_row == row ? "Yes" : "No", latency);

    *total_latency = latency;
    return address; // Return the accessed address
}

// Function to print the state of the DRAM banks
void print_dram_state(DRAM *dram) {
    printf("DRAM State:\n");
    printf("-------------------------------------------------------------\n");
    printf("Bank | Active Row | Stored Addresses        | Last Accessed Time\n");
    printf("-------------------------------------------------------------\n");
    for (int i = 0; i < BANKS; i++) {
        printf("%-4d | %-10d |", i, dram->banks[i].active_row);
        for (int j = 0; j < ROWS; j++) {
            for (int k = 0; k < dram->banks[i].row_counts[j]; k++) {
                printf(" 0x%02x", dram->banks[i].stored_addresses[j][k]);
            }
        }
        printf(" | %-18d\n", dram->banks[i].time_last_accessed);
    }
    printf("-------------------------------------------------------------\n");
    printf("Global Time: %d\n", dram->time);
}

// Function to visualize the DRAM access for multiple addresses
void visualize_dram_access(DRAMController *controller, uint8_t addresses[], int num_addresses) {
    int total_latency;
    printf("Bank | Row  | Column | Address     | Row Active | Latency\n");
    printf("-------------------------------------------------------------\n");
    for (int i = 0; i < num_addresses; i++) {
        access_dram(controller, addresses[i], &total_latency);
    }
    printf("-------------------------------------------------------------\n");
    print_dram_state(controller->dram);
}

// Main function to demonstrate the DRAM access simulation
int main() {
    DRAM dram; // Declare a DRAM structure
    DRAMController controller; // Declare a DRAM controller structure

    // Example addresses to fill row 0 in bank 0 and then move to another bank/row
    uint8_t addresses[] = {
        0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09,
        0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, // These will fill row 0 in bank 0
        0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19,
        0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F  // These should go to another bank or row
    };

    int num_addresses = sizeof(addresses) / sizeof(addresses[0]);

    // Initialize the DRAM
    init_dram(&dram);
    // Initialize the DRAM controller with row interleaving
    init_dram_controller(&controller, &dram, row_interleaving);

    // Access various addresses using row interleaving
    printf("Using Row Interleaving:\n");
    visualize_dram_access(&controller, addresses, num_addresses);

    // Print the last accessed address
    printf("Last accessed address: 0x%02x\n", last_accessed_address);

    // Re-initialize the DRAM
    init_dram(&dram);
    // Re-initialize the DRAM controller with cache block interleaving
    init_dram_controller(&controller, &dram, cache_block_interleaving);

    // Access various addresses using cache block interleaving
    printf("\nUsing Cache Block Interleaving:\n");
    visualize_dram_access(&controller, addresses, num_addresses);

    // Print the last accessed address
    printf("Last accessed address: 0x%02x\n", last_accessed_address);

    // Free the allocated memory in the DRAM structure
    free_dram(&dram);

    return 0;
}
