#define _CRT_SECURE_NO_WARNINGS
#include "dram_simulation.h"
#include "extract_address_trace.h" // Include the new header file for extract_address_trace.c
#include <stdio.h>
#include <stdint.h> // for uint32_t
#include <stdlib.h> // for malloc and free

int main() {
    unsigned int *addresses;
    int num_addresses = extract_addresses_from_file("linpack_val.txt", &addresses);
    if (num_addresses == 0) {
        printf("No addresses extracted. Exiting.\n");
        return 1;
    }

    int total_latency = 0;
    int latency;

    // Call the simulate_dram_access function with each address
    for (int i = 0; i < num_addresses; i++) {
        latency = simulate_dram_access(addresses[i]);
        total_latency += latency;

        // Print the accessed address and latency
        printf("Accessed address: 0x%08x, Latency: %d cycles\n", addresses[i], latency);
    }

    // Print the total latency
    printf("Total latency: %d cycles\n", total_latency);

    // Free the allocated memory
    free(addresses);

    return 0;
}
