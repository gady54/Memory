#define _CRT_SECURE_NO_WARNINGS
#include "dram_simulation.h" // Include the new header file for dram_simulation.c
#include "extract_address_trace.h" // Include the new header file for extract_address_trace.c
#include <stdio.h>
#include <stdint.h> // for uint32_t
#include <stdlib.h> // for malloc and free
#include "cache_simulation.h" // Include the new header file for cache_simulation.c

int main() {
    unsigned int* addresses;
    int num_addresses = extract_addresses_from_file("linpack_val.txt", &addresses);
    if (num_addresses == 0) {
        printf("No addresses extracted. Exiting.\n");
        return 1;
    }

    

    // Process each address through the cache simulation
    for (int i = 0; i < num_addresses; i++) {
        simulate_cache(addresses[i]);
    }

    // Print final simulation results
    print_simulation_results();

    // Free the allocated memory
    free(addresses);

    return 0;
}
