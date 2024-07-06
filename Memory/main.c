#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

// Structure for Cache statistics
typedef struct {
    int hits; // Hit count
    int misses; // Miss count
    int accesses; // Access count
    // Additional data for cache management
} CacheStats;

// Structure for Cache configuration
typedef struct {
    int size;
    int block;
    int associativity;
    int hit_time;
    int write_allocate;
    int write_back;
    char replacement_policy[256];
    int bus_width_hierarchy;
    CacheStats stats;
    int* memory; // Pointer to the cache memory array
} CacheConfig;

// Structure for DDR configuration
typedef struct {
    int bus_width;
    int channels;
    int DIMMs;
    int num_banks;
    int RAS_time;
    int CAS_time;
    int ranks;
    int access_time;
    int transfer_time;
    char mapping_method[256];
    int** banks; // Pointer to an array of pointers for banks
} DDRConfig;

// Function to read cache configuration from a file
CacheConfig readcacheconfig(FILE* filename, const char* cache) {
    CacheConfig cacheconfig;
    char buffer[256];
    char num[10] = { 0 };  // Ensure num is initialized to avoid garbage values
    while (fgets(buffer, sizeof(buffer), filename)) {
        char* pos = strstr(buffer, cache);
        if (pos) {
            int i, j;
            for (i = (int)(pos - buffer) + (int)strlen(cache), j = 0; i < (int)strlen(buffer); i++) {
                if (isdigit(buffer[i])) {
                    num[j++] = buffer[i];
                }
            }
            num[j] = '\0';  // Ensure num is null-terminated

            if (strstr(buffer, "KB")) {
                cacheconfig.size = atoi(num) * 1024;
            }
            else if (strstr(buffer, "MB")) {
                cacheconfig.size = atoi(num) * 1048576;
            }
            else {
                cacheconfig.size = atoi(num);
            }
            fscanf(filename, "%*[^=]=%d\n", &cacheconfig.block);
            fscanf(filename, "%*[^=]=%d\n", &cacheconfig.associativity);
            fscanf(filename, "%*[^=]=%d\n", &cacheconfig.hit_time);
            fscanf(filename, "%*[^=]=%d\n", &cacheconfig.write_allocate);
            fscanf(filename, "%*[^=]=%d\n", &cacheconfig.write_back);
            fscanf(filename, "%*[^=]=%s\n", cacheconfig.replacement_policy);
            fscanf(filename, "%*[^=]=%d\n", &cacheconfig.bus_width_hierarchy);
            break;
        }
    }
    int num_elements = cacheconfig.size / cacheconfig.block;
    // Allocate memory for cache
    cacheconfig.memory = (int*)malloc(num_elements * sizeof(int));

    // Check if the memory has been successfully allocated
    if (cacheconfig.memory == NULL) {
        fprintf(stderr, "Memory allocation failed\n");
        exit(EXIT_FAILURE);
    }

    // Use the allocated memory (example: initializing the array)
    for (int i = 0; i < num_elements; i++) {
        cacheconfig.memory[i] = i;
    }

    return cacheconfig;
}

// Function to read DDR configuration from a file
DDRConfig readDDRConfig(FILE* filename) {
    DDRConfig ddrconfig;
    fscanf(filename, "%*[^=]=%d\n", &ddrconfig.bus_width);
    fscanf(filename, "%*[^=]=%d\n", &ddrconfig.channels);
    fscanf(filename, "%*[^=]=%d\n", &ddrconfig.DIMMs);
    fscanf(filename, "%*[^=]=%d\n", &ddrconfig.num_banks);
    fscanf(filename, "%*[^=]=%d\n", &ddrconfig.RAS_time);
    fscanf(filename, "%*[^=]=%d\n", &ddrconfig.CAS_time);
    fscanf(filename, "%*[^=]=%d\n", &ddrconfig.ranks);
    fscanf(filename, "%*[^=]=%s\n", ddrconfig.mapping_method);

    // Allocate memory for banks
    ddrconfig.banks = (int**)malloc(ddrconfig.num_banks * sizeof(int*));
    if (ddrconfig.banks == NULL) {
        perror("Failed to allocate memory for banks");
        exit(EXIT_FAILURE);
    }

    // Allocate memory for each bank
    for (int i = 0; i < ddrconfig.num_banks; i++) {
        ddrconfig.banks[i] = (int*)malloc((1 << 20) * sizeof(int)); // Assuming 1 MB per bank
        if (ddrconfig.banks[i] == NULL) {
            perror("Failed to allocate memory for bank");
            exit(EXIT_FAILURE);
        }
    }
    return ddrconfig;
}

// Function to read the complete configuration
void readConfiguration(const char* filename, CacheConfig* l1, CacheConfig* l2, CacheConfig* l3, DDRConfig* ddr) {
    FILE* file = fopen(filename, "r");
    if (!file) {
        perror("Failed to open configuration file");
        exit(EXIT_FAILURE);
    }

    *l1 = readcacheconfig(file, "L1");
    *l2 = readcacheconfig(file, "L2");
    *l3 = readcacheconfig(file, "L3");

    *ddr = readDDRConfig(file);

    fclose(file);
}

/*int main()
{
    CacheConfig l1, l2, l3;
    DDRConfig ddr;
    readConfiguration("Configuration.txt", &l1, &l2, &l3, &ddr);

    // Free allocated memory for cache
    free(l1.memory);
    free(l2.memory);
    free(l3.memory);

    // Free allocated memory for banks
    for (int i = 0; i < ddr.num_banks; i++) {
        free(ddr.banks[i]);
    }
    free(ddr.banks);

    return 0;
}*/