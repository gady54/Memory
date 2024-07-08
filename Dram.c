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
#define CACHE_BLOCK_SIZE 64 // Assuming 64 bytes cache block
#define MAX_PENDING_REQUESTS 16 // Maximum number of pending requests in the scheduler

// Global variable to store the last accessed address
uint32_t last_accessed_address = 0;

// Structure representing a DRAM bank
typedef struct {
    int active_row;         // Currently active row in the bank (-1 if no row is active)
    int time_last_accessed; // Time when the bank was last accessed
    uint32_t **stored_addresses; // Array to store addresses in the bank
    int *row_counts;        // Array to count the number of addresses per row
} DRAMBank;

// Structure representing the entire DRAM
typedef struct {
    DRAMBank banks[BANKS];  // Array of banks in the DRAM
    int time;               // Global time counter
} DRAM;

// Structure representing a memory request
typedef struct {
    uint32_t address;
    int bank;
    int row;
    int col;
} MemoryRequest;

// Structure representing the DRAM Controller
typedef struct {
    DRAM *dram;             // Pointer to the DRAM
    void (*address_mapping)(uint32_t, int*, int*, int*); // Function pointer for address mapping
    MemoryRequest pending_requests[MAX_PENDING_REQUESTS]; // Queue of pending requests
    int num_pending_requests; // Number of pending requests
    MemoryRequest *address_map; // Mapping table for addresses
    int address_map_size; // Size of the address map
} DRAMController;

// Function to initialize the DRAM structure
void init_dram(DRAM *dram) {
    for (int i = 0; i < BANKS; i++) {
        dram->banks[i].active_row = -1;         // Set all banks' active rows to -1 (no row is active)
        dram->banks[i].time_last_accessed = -1; // Set all banks' last accessed time to -1
        dram->banks[i].stored_addresses = (uint32_t **)malloc(ROWS * sizeof(uint32_t *));
        for (int j = 0; j < ROWS; j++) {
            dram->banks[i].stored_addresses[j] = (uint32_t *)malloc(COLUMNS * sizeof(uint32_t));
            for (int k = 0; k < COLUMNS; k++) {
                dram->banks[i].stored_addresses[j][k] = 0; // Initialize all addresses to 0
            }
        }
        dram->banks[i].row_counts = (int *)malloc(ROWS * sizeof(int));
        for (int j = 0; j < ROWS; j++) {
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
        free(dram->banks[i].row_counts);
    }
}

// Function to initialize the DRAM Controller
void init_dram_controller(DRAMController *controller, DRAM *dram, void (*address_mapping)(uint32_t, int*, int*, int*)) {
    controller->dram = dram;
    controller->address_mapping = address_mapping;
    controller->num_pending_requests = 0;
    controller->address_map_size = 0;
    controller->address_map = NULL;
}

// Function to allocate the address map
void allocate_address_map(DRAMController *controller, int size) {
    controller->address_map = (MemoryRequest *)malloc((size_t)size * sizeof(MemoryRequest));
    controller->address_map_size = size;
}

// Function for row interleaving: translate an address to bank, row, and column
void row_interleaving(uint32_t address, int *bank, int *row, int *col) {
    *col = (int)((address >> 2) & 0x3FF);        // Column is determined by bits 2-11 (10 bits)
    *bank = (int)((address >> 12) & 0x3);        // Bank is determined by bits 12-13 (2 bits)
    *row = (int)((address >> 14) & 0x3FFFF);     // Row is determined by bits 14-31 (18 bits)
}

// Function for cache block interleaving: translate an address to bank, row, and column
void cache_block_interleaving(uint32_t address, int *bank, int *row, int *col) {
    *col = (int)((address >> 2) & 0xFF); // Low column bits (8 bits)
    *bank = (int)((address >> 10) & 0x3); // Bank bits (2 bits)
    *row = (int)((address >> 12) & 0x3FFFF); // Row bits (18 bits)
    // High column bits (2 bits) are included in *col calculation
}

// Function to check if an address is present in the bank
int is_address_in_bank(DRAMBank *bank, uint32_t address, int row, int col) {
    return bank->stored_addresses[row][col] == address;
}

// Function to save address in the bank
void save_address_in_bank(DRAMBank *bank, uint32_t address, int row, int col) {
    bank->stored_addresses[row][col] = address;
    bank->row_counts[row]++;
}

// Function to update the mapping table
void update_address_mapping(DRAMController *controller, uint32_t address, int bank, int row, int col) {
    for (int i = 0; i < controller->address_map_size; i++) {
        if (controller->address_map[i].address == address) {
            controller->address_map[i].bank = bank;
            controller->address_map[i].row = row;
            controller->address_map[i].col = col;
            return;
        }
    }
    // If the address is not already in the map, add it
    controller->address_map[controller->address_map_size++] = (MemoryRequest){address, bank, row, col};
}

// Function to find the current mapping of an address
bool find_address_mapping(DRAMController *controller, uint32_t address, int *bank, int *row, int *col) {
    for (int i = 0; i < controller->address_map_size; i++) {
        if (controller->address_map[i].address == address) {
            *bank = controller->address_map[i].bank;
            *row = controller->address_map[i].row;
            *col = controller->address_map[i].col;
            return true;
        }
    }
    return false;
}

// Function to find a new bank or row if the current row is full or has a conflict
void find_new_bank_or_row(DRAMController *controller, uint32_t address, int *bank, int *row, int col) {
    DRAM *dram = controller->dram;
    for (int b = 0; b < BANKS; b++) {
        for (int r = 0; r < ROWS; r++) {
            if (dram->banks[b].row_counts[r] < COLUMNS && dram->banks[b].stored_addresses[r][col] == 0) {
                *bank = b;
                *row = r;
                update_address_mapping(controller, address, b, r, col);
                return;
            }
        }
    }
    printf("All banks and rows are full or have conflicts.\n");
}

// Hypothetical function to simulate sending the address to the cache
void send_to_cache(uint32_t address) {
    printf("Address 0x%08x is sent to the cache.\n", address);
}

// Function to access a specific address in DRAM through the controller
uint32_t access_dram(DRAMController *controller, uint32_t address, int *total_latency) {
    int bank, row, col;
    int latency = 0;
    DRAM *dram = controller->dram;

    // Check if the address has already been remapped
    if (!find_address_mapping(controller, address, &bank, &row, &col)) {
        // If not, translate the address using the provided mapping function
        controller->address_mapping(address, &bank, &row, &col);
        // Save the initial mapping in the address map
        update_address_mapping(controller, address, bank, row, col);
    }

    // Access the specified bank, row, and column
    DRAMBank *current_bank = &dram->banks[bank];

    // Store the previously active row
    int previous_active_row = current_bank->active_row;

    // Check if the requested address is already in the bank
    if (is_address_in_bank(current_bank, address, row, col)) {
        // Send the address to the cache if already present
        send_to_cache(address);
        latency += CAS_TIME; // Add CAS latency for accessing the column
    } else {
        // Check if the current row is full or has a conflict
        if (current_bank->row_counts[row] >= COLUMNS || current_bank->stored_addresses[row][col] != 0) {
            printf("Row %d in Bank %d is full or has a conflict at column %d. Finding new bank or row.\n", row, bank, col);
            find_new_bank_or_row(controller, address, &bank, &row, col);
            current_bank = &dram->banks[bank]; // Update the current bank reference
        }

        // Save the address in the bank if not present
        save_address_in_bank(current_bank, address, row, col);

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

    printf("%-4d | %-4d | %-6d | 0x%08x | %-9s | %-7d cycles\n",
           bank, row, col, address, previous_active_row == row ? "NO" : "YES", latency);

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
            for (int k = 0; k < COLUMNS; k++) {
                if (dram->banks[i].stored_addresses[j][k] != 0) {
                    printf(" 0x%08x", dram->banks[i].stored_addresses[j][k]);
                }
            }
        }
        printf(" | %-18d\n", dram->banks[i].time_last_accessed);
    }
    printf("-------------------------------------------------------------\n");
    printf("Global Time: %d\n", dram->time);
}

// Scheduler function to prioritize memory requests
void schedule_requests(DRAMController *controller) {
    // Sort pending requests based on row number to minimize row conflicts
    for (int i = 0; i < controller->num_pending_requests - 1; i++) {
        for (int j = i + 1; j < controller->num_pending_requests; j++) {
            if (controller->pending_requests[i].row > controller->pending_requests[j].row) {
                MemoryRequest temp = controller->pending_requests[i];
                controller->pending_requests[i] = controller->pending_requests[j];
                controller->pending_requests[j] = temp;
            }
        }
    }
}

// Function to add a memory request to the scheduler
void add_request_to_scheduler(DRAMController *controller, uint32_t address) {
    if (controller->num_pending_requests < MAX_PENDING_REQUESTS) {
        int bank, row, col;
        if (find_address_mapping(controller, address, &bank, &row, &col)) {
            controller->pending_requests[controller->num_pending_requests++] = (MemoryRequest){address, bank, row, col};
        } else {
            controller->address_mapping(address, &bank, &row, &col);
            update_address_mapping(controller, address, bank, row, col);
            controller->pending_requests[controller->num_pending_requests++] = (MemoryRequest){address, bank, row, col};
        }
    } else {
        printf("Scheduler queue is full. Cannot add more requests.\n");
    }
}

// Function to visualize the DRAM access for multiple addresses
void visualize_dram_access(DRAMController *controller, uint32_t addresses[], int num_addresses) {
    int total_latency;
    printf("Bank | Row  | Column | Address     | Row Active | Latency\n");
    printf("-------------------------------------------------------------\n");
    for (int i = 0; i < num_addresses; i++) {
        add_request_to_scheduler(controller, addresses[i]);
    }

    // Schedule and process requests
    schedule_requests(controller);
    for (int i = 0; i < controller->num_pending_requests; i++) {
        access_dram(controller, controller->pending_requests[i].address, &total_latency);
    }
    printf("-------------------------------------------------------------\n");
    print_dram_state(controller->dram);
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
    for (int i = 0; i < count; i++) {
        fscanf(file, "%x", &(*addresses)[i]);
    }

    fclose(file);
    return count;
}

// Main function to demonstrate the DRAM access simulation
int main() {
    DRAM dram; // Declare a DRAM structure
    DRAMController controller; // Declare a DRAM controller structure

    uint32_t *addresses;
    int num_addresses = read_addresses_from_file("address.txt", &addresses);

    if (num_addresses == 0) {
        printf("No addresses to process. Exiting.\n");
        return 1;
    }

    int choice;
    printf("Choose address mapping method:\n");
    printf("1. Row Interleaving\n");
    printf("2. Cache Block Interleaving\n");
    printf("Enter your choice: ");
    scanf("%d", &choice);

    // Initialize the DRAM
    init_dram(&dram);

    // Initialize the DRAM controller with the chosen address mapping
    if (choice == 1) {
        init_dram_controller(&controller, &dram, row_interleaving);
        printf("Using Row Interleaving:\n");
    } else if (choice == 2) {
        init_dram_controller(&controller, &dram, cache_block_interleaving);
        printf("Using Cache Block Interleaving:\n");
    } else {
        printf("Invalid choice. Exiting.\n");
        free(addresses);
        return 1;
    }

    // Allocate address map
    allocate_address_map(&controller, num_addresses);

    // Access various addresses using the chosen address mapping
    visualize_dram_access(&controller, addresses, num_addresses);

    // Print the last accessed address
    printf("Last accessed address: 0x%08x\n", last_accessed_address);

    // Free the allocated memory
    free_dram(&dram);
    free(controller.address_map);
    free(addresses);

    return 0;
}
