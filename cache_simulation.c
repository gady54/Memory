#include "cache_simulation.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "dram_simulation.h"

//----------------------------------------------------------------//
//Global Variables
unsigned int hits = 0;
unsigned int misses = 0;
unsigned int total_commands = 0;
unsigned int cycles = 0;
unsigned int DRAM_Cycle = 0;
unsigned int oldL1Address;
unsigned int oldL2Address;
unsigned int oldL3Address;
// end of global variables
//----------------------------------------------------------------

unsigned int get_total_cycles() {
    return cycles;
}

unsigned int get_hits() {
    return hits;
}

unsigned int get_misses() {
    return misses;
}

unsigned int get_total_commands() {
    return total_commands;
}

CacheLine* initialize_cache(int cache_size) {
    int num_lines = cache_size / BLOCK_SIZE;
    CacheLine* cache = (CacheLine*)malloc((size_t)num_lines * sizeof(CacheLine));
    if (cache == NULL) {
        fprintf(stderr, "Memory allocation failed\n");
        exit(EXIT_FAILURE);
    }
    for (int i = 0; i < num_lines; i++) {
        cache[i].valid = 0;
        cache[i].tag = 0;
    }
    return cache;
}

void update_cache(CacheLine* cache, unsigned int address, int cache_size) {
    int index_bits = (int)log2(cache_size / BLOCK_SIZE);
    unsigned int index_mask = (1U << index_bits) - 1;
    unsigned int index = (address >> (int)log2(BLOCK_SIZE)) & index_mask;
    unsigned int tag = address >> (index_bits + (int)log2(BLOCK_SIZE));

    cache[index].valid = 1;
    cache[index].tag = tag;
}

void reset_cache(CacheLine* cache, unsigned int address, int cache_size) {
    int index_bits = (int)log2(cache_size / BLOCK_SIZE);
    unsigned int index_mask = (1U << index_bits) - 1;
    unsigned int index = (address >> (int)log2(BLOCK_SIZE)) & index_mask;

    cache[index].valid = 0;
    cache[index].tag = 0;
}

void update_cache_L1(CacheLine* L1, unsigned int address) {
    update_cache(L1, address, L1_SIZE);
}

void update_cache_L2(CacheLine* L2, unsigned int address) {
    update_cache(L2, address, L2_SIZE);
}

void update_cache_L3(CacheLine* L3, unsigned int address) {
    update_cache(L3, address, L3_SIZE);
}

int is_in_cache(CacheLine* cache, unsigned int address, int cache_size) {
    int index_bits = (int)log2(cache_size / BLOCK_SIZE);
    unsigned int index_mask = (1U << index_bits) - 1;
    unsigned int index = (address >> (int)log2(BLOCK_SIZE)) & index_mask;
    unsigned int tag = address >> (index_bits + (int)log2(BLOCK_SIZE));

    return cache[index].valid && (cache[index].tag == tag);
}

int is_valid_bit_set(CacheLine* cache, unsigned int address, int cache_size) {
    int index_bits = (int)log2(cache_size / BLOCK_SIZE);
    unsigned int index_mask = (1U << index_bits) - 1;
    unsigned int index = (address >> (int)log2(BLOCK_SIZE)) & index_mask;

    return cache[index].valid;
}

int is_in_cache_L1(CacheLine* L1, unsigned int address) {
    return is_in_cache(L1, address, L1_SIZE);
}

int is_in_cache_L2(CacheLine* L2, unsigned int address) {
    return is_in_cache(L2, address, L2_SIZE);
}

int is_in_cache_L3(CacheLine* L3, unsigned int address) {
    return is_in_cache(L3, address, L3_SIZE);
}

void print_cache_values(CacheLine* cache, int cache_size, const char* cache_name) {
    int num_lines = cache_size / BLOCK_SIZE;
    printf("%s Cache Contents:\n", cache_name);
    printf("Index | Valid | Tag\n");
    for (int i = 0; i < num_lines; i++) {
        if (cache[i].valid != 0)
            printf("%5d | %5d | %08X\n", i, cache[i].valid, cache[i].tag);
    }
}

void print_cache_info(int cache_size, const char* cache_name) {
    int num_lines = cache_size / BLOCK_SIZE;
    int offset_bits = (int)log2(BLOCK_SIZE);
    int index_bits = (int)log2(num_lines);
    int tag_bits = ADDRESS_BITS - index_bits - offset_bits;
    printf("%s: %d Index Bits, %d Tag Bits, %d Offset Bits, 1 Valid Bit\n",
           cache_name, index_bits, tag_bits, offset_bits);
}

void print_index_and_tag(unsigned int address, int cache_size, const char* cache_name) {
    int index_bits = (int)log2(cache_size / BLOCK_SIZE);
    unsigned int index_mask = (1U << index_bits) - 1;
    unsigned int index = (address >> (int)log2(BLOCK_SIZE)) & index_mask;
    unsigned int tag = address >> (index_bits + (int)log2(BLOCK_SIZE));

    printf("%s Address %08X: Index = %u, Tag = %08X\n", cache_name, address, index, tag);
}

unsigned int get_full_address(unsigned int index, unsigned int tag, int cache_size) {
    int index_bits = (int)log2(cache_size / BLOCK_SIZE);
    int offset_bits = (int)log2(BLOCK_SIZE);

    unsigned int address = tag << (index_bits + offset_bits);
    address |= (index << offset_bits);

    return address;
}

void moveToDram(unsigned int address) {
     cycles += (unsigned int)simulate_dram_access(address);
    printf("moveToDram , %08X\n", address);
}

void hit_miss_finder(CacheLine* L1, CacheLine* L2, CacheLine* L3, unsigned int address) {
    if (is_in_cache_L1(L1, address)) {
        printf("Hit on L1 for address %08X\n", address);
        hits++;
        cycles += L1_cycles;
    } else if (is_in_cache_L2(L2, address)) {
        printf("Hit on L2 for address %08X\n", address);
        hits++;
        cycles += L2_cycles + L1_cycles;
    } else if (is_in_cache_L3(L3, address)) {
        printf("Hit on L3 for address %08X\n", address);
        hits++;
        cycles += L3_cycles + L1_cycles + L2_cycles;
    } else {
        printf("Not found in cache. Upload from DRAM %08X\n", address);
        misses++;
        cycles += (unsigned int)simulate_dram_access(address) + L3_cycles + L1_cycles + L2_cycles;
    }
}

void LRU(CacheLine* L1, CacheLine* L2, CacheLine* L3, unsigned int address) {
    if (!is_valid_bit_set(L1, address, L1_SIZE)) {
        update_cache_L1(L1, address);
        return;
    } else {
        if (is_in_cache_L1(L1, address))
            return;
        else {
            unsigned int l1_index = (address >> (int)log2(BLOCK_SIZE)) & ((1U << (int)log2(L1_SIZE / BLOCK_SIZE)) - 1);
            unsigned int l1_storedTag = L1[l1_index].tag;
            oldL1Address = get_full_address(l1_index, l1_storedTag, L1_SIZE);
            update_cache_L1(L1, address);
            if (!is_valid_bit_set(L2, oldL1Address, L2_SIZE)) {
                update_cache_L2(L2, oldL1Address);
            } else {
                unsigned int l2_index = (oldL1Address >> (int)log2(BLOCK_SIZE)) & ((1U << (int)log2(L2_SIZE / BLOCK_SIZE)) - 1);
                unsigned int l2_storedTag = L2[l2_index].tag;
                oldL2Address = get_full_address(l2_index, l2_storedTag, L2_SIZE);
                update_cache_L2(L2, oldL1Address);
                if (!is_valid_bit_set(L3, oldL2Address, L3_SIZE)) {
                    update_cache_L3(L3, oldL2Address);
                } else {
                    unsigned int l3_index = (oldL2Address >> (int)log2(BLOCK_SIZE)) & ((1U << (int)log2(L3_SIZE / BLOCK_SIZE)) - 1);
                    unsigned int l3_storedTag = L3[l3_index].tag;
                    oldL3Address = get_full_address(l3_index, l3_storedTag, L3_SIZE);
                    update_cache_L3(L3, oldL2Address);
                    if (address != oldL3Address)
                        moveToDram(oldL3Address);
                }
            }
        }
    }

    if (is_in_cache_L2(L2, address)) {
        reset_cache(L2, address, L2_SIZE);
    }

    if (is_in_cache_L3(L3, address)) {
        reset_cache(L3, address, L3_SIZE);
    }
}

void full_cache_logic(CacheLine* L1, CacheLine* L2, CacheLine* L3, unsigned int address) {
    hit_miss_finder(L1, L2, L3, address);
    LRU(L1, L2, L3, address);
    total_commands++;
}

void print_simulation_results() {
    printf("Total Hits: %u, Misses: %u, Total Commands: %u, Total Cycles: %u\n",
           get_hits(), get_misses(), get_total_commands(), get_total_cycles());
}
