#define _CRT_SECURE_NO_WARNINGS
#include "dram_simulation.h" // Include the new header file for dram_simulation.c
#include "extract_address_trace.h" // Include the new header file for extract_address_trace.c
#include <stdio.h>
#include <stdint.h> // for uint32_t
#include <stdlib.h> // for malloc and free
#include "cache_simulation.h" // Include the new header file for cache_simulation.h
#include "cache_simulation.c" // Include the new header file for cache_simulation.c

#define A 0x1A2B3C00
#define B 0xCA1B3C00
#define C 0x3A2B3C00
#define D 0x2A2B3C00
#define E 0xCA1B2C00

int main() {
    /*/
    unsigned int* addresses;
    int num_addresses = extract_addresses_from_file("linpack_val.txt", &addresses);
    if (num_addresses == 0) {
        printf("No addresses extracted. Exiting.\n");
        return 1;
    }

    /*/
    CacheLine* L1 = initialize_cache(L1_SIZE);
    CacheLine* L2 = initialize_cache(L2_SIZE);
    CacheLine* L3 = initialize_cache(L3_SIZE);

     unsigned int test_addresses[] = {
        B, A, C, E, D,
        B, A, D, E, C,
        A, D, E, C, B,
        E, C, A, D, B
    };

    // Process each address through the cache simulation
    for (int i = 0; i < 20; i++) {
        full_cache_logic(L1, L2, L3, test_addresses[i]);
    }

    // Print final simulation results
    print_simulation_results();

    // Free the allocated memory
    //free(addresses);

    return 0;
}
