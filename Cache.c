#include <stdio.h>
#include <stdlib.h>
#include <math.h>  // For log2 function

#define L1_SIZE (1 * 1024) 
#define L2_SIZE (1* 1024)  
#define L3_SIZE (1 * 1024) 
#define BLOCK_SIZE 64  // Assuming block size is 64 bytes
#define ADDRESS_BITS 32  // Assuming a 32-bit address space

#define L1_cycels 1
#define L2_cycels 2
#define L3_cycels 3
#define DRAM_Cycle 10

//----------------------------------------------------------------//
//Global Variables
unsigned int hits = 0; //Sum all hits
unsigned int misses = 0; // Sum all misses
unsigned int total_commands = 0; //Sum all commands load and store
unsigned int cycles = 0; //Sum all

unsigned int oldL1Address;
unsigned int oldL2Address;
unsigned int oldL3Address;
//end of global variables
//----------------------------------------------------------------

/// @brief This struct represents a cache line.
typedef struct
{
    int valid;        // Valid bit
    unsigned int tag; // Tag
} CacheLine;

/// @brief This function allocate memory for a cache line and return a pointer.
/// @param cache_size 
/// @return 
CacheLine* initialize_cache(int cache_size) {
    int num_lines = cache_size / BLOCK_SIZE;
    CacheLine* cache = (CacheLine*)malloc(num_lines * sizeof(CacheLine));
    if (cache == NULL) {
        fprintf(stderr, "Memory allocation failed\n");
        exit(EXIT_FAILURE);
    }
    for (int i = 0; i < num_lines; i++) {
        cache[i].valid = 0; // Set all valid bits to 0
        cache[i].tag = 0;   // Initialize tag to 0
    }
    return cache;
}

/// @brief This function can take any cache Lx and update the cache.
/// @param cache 
/// @param cache_size 
/// @param cache_name 
void update_cache(CacheLine* cache, unsigned int address, int cache_size) {
    int index_bits = (int)log2(cache_size / BLOCK_SIZE);
    int index_mask = (1 << index_bits) - 1;
    int index = (address >> (int)log2(BLOCK_SIZE)) & index_mask;
    unsigned int tag = address >> (index_bits + (int)log2(BLOCK_SIZE));

    cache[index].valid = 1;
    cache[index].tag = tag;
}


/// @brief This function update L1 cache 
/// @param L1 
/// @param address 
void update_cache_L1(CacheLine* L1, unsigned int address) {
    int index_bits = (int)log2(L1_SIZE / BLOCK_SIZE); // L1 specific calculations
    int index_mask = (1 << index_bits) - 1;
    int index = (address >> (int)log2(BLOCK_SIZE)) & index_mask;
    unsigned int tag = address >> (index_bits + (int)log2(BLOCK_SIZE));

    L1[index].valid = 1;
    L1[index].tag = tag;
}

/// @brief This function update L2 cache 
/// @param L2 
/// @param address 
void update_cache_L2(CacheLine* L2, unsigned int address) {
    int index_bits = (int)log2(L2_SIZE / BLOCK_SIZE); // L2 specific calculations
    int index_mask = (1 << index_bits) - 1;
    int index = (address >> (int)log2(BLOCK_SIZE)) & index_mask;
    unsigned int tag = address >> (index_bits + (int)log2(BLOCK_SIZE));

    L2[index].valid = 1;
    L2[index].tag = tag;
}

/// @brief This function update L3 cache 
/// @param L3 
/// @param address 
void update_cache_L3(CacheLine* L3, unsigned int address) {
    int index_bits = (int)log2(L3_SIZE / BLOCK_SIZE); // L3 specific calculations
    int index_mask = (1 << index_bits) - 1;
    int index = (address >> (int)log2(BLOCK_SIZE)) & index_mask;
    unsigned int tag = address >> (index_bits + (int)log2(BLOCK_SIZE));

    L3[index].valid = 1;
    L3[index].tag = tag;
}

/// @brief This function check if Lx cache is valid and have the same tag.
/// @param cache 
/// @param address 
/// @param cache_size 
/// @return 
int is_in_cache(CacheLine* cache, unsigned int address, int cache_size) {
    int index_bits = (int)log2(cache_size / BLOCK_SIZE);
    int index_mask = (1 << index_bits) - 1;
    int index = (address >> (int)log2(BLOCK_SIZE)) & index_mask;
    unsigned int tag = address >> (index_bits + (int)log2(BLOCK_SIZE));

    return cache[index].valid && (cache[index].tag == tag);
}

/// @brief Check if the valid bit is set for the given address in the specified cache.
/// @param cache The cache in which to check the address.
/// @param address The memory address to check.
/// @param cache_size The size of the cache.
/// @return 1 if the valid bit is set for the given address, 0 otherwise.
    int is_valid_bit_set(CacheLine* cache, unsigned int address, int cache_size) {
    int num_lines = cache_size / BLOCK_SIZE;  // Number of lines in the cache
    int index_bits = (int)log2(num_lines);    // Calculate the number of index bits needed
    int index_mask = (1 << index_bits) - 1;   // Create a mask to extract the index
    int index = (address >> (int)log2(BLOCK_SIZE)) & index_mask; // Calculate the index from the address

    return cache[index].valid; // Return the state of the valid bit for the calculated index
}


/// @brief Check if the given address is in L1 cache.
/// @param L1 The L1 cache.
/// @param address The memory address to check.
/// @return 1 if in cache and valid, 0 otherwise.
int is_in_cache_L1(CacheLine* L1, unsigned int address) {
    int index_bits = (int)log2(L1_SIZE / BLOCK_SIZE); // L1 specific calculations
    int index_mask = (1 << index_bits) - 1;
    int index = (address >> (int)log2(BLOCK_SIZE)) & index_mask;
    unsigned int tag = address >> (index_bits + (int)log2(BLOCK_SIZE));

    return L1[index].valid && (L1[index].tag == tag);
}

/// @brief Check if the given address is in L2 cache.
/// @param L2 The L2 cache.
/// @param address The memory address to check.
/// @return 1 if in cache and valid, 0 otherwise.
int is_in_cache_L2(CacheLine* L2, unsigned int address) {
    int index_bits = (int)log2(L2_SIZE / BLOCK_SIZE); // L2 specific calculations
    int index_mask = (1 << index_bits) - 1;
    int index = (address >> (int)log2(BLOCK_SIZE)) & index_mask;
    unsigned int tag = address >> (index_bits + (int)log2(BLOCK_SIZE));

    return L2[index].valid && (L2[index].tag == tag);
}

/// @brief Check if the given address is in L3 cache.
/// @param L3 The L3 cache.
/// @param address The memory address to check.
/// @return 1 if in cache and valid, 0 otherwise.
int is_in_cache_L3(CacheLine* L3, unsigned int address) {
    int index_bits = (int)log2(L3_SIZE / BLOCK_SIZE); // L3 specific calculations
    int index_mask = (1 << index_bits) - 1;
    int index = (address >> (int)log2(BLOCK_SIZE)) & index_mask;
    unsigned int tag = address >> (index_bits + (int)log2(BLOCK_SIZE));

    return L3[index].valid && (L3[index].tag == tag);
}


void print_cache_values(CacheLine* cache, int cache_size, const char* cache_name) {
    int num_lines = cache_size / BLOCK_SIZE;
    printf("%s Cache Contents:\n", cache_name);
    printf("Index | Valid | Tag\n");
    for (int i = 0; i < num_lines; i++) {
        printf("%5d | %5d | %08X\n", i, cache[i].valid, cache[i].tag);
    }
}

/// @brief By given a cache size the name of the cache the function will print how many Index Bits, Tag Bits,  Offset Bits, 1 Valid Bit
/// @param cache_size 
/// @param cache_name 
void print_cache_info(int cache_size, const char* cache_name) {
    int num_lines = cache_size / BLOCK_SIZE;
    int offset_bits = (int)log2(BLOCK_SIZE);
    int index_bits = (int)log2(num_lines);
    int tag_bits = ADDRESS_BITS - index_bits - offset_bits;
    printf("%s: %d Index Bits, %d Tag Bits, %d Offset Bits, 1 Valid Bit\n",
        cache_name, index_bits, tag_bits, offset_bits);
}

/// @brief Function to print the index and tag for a given address and cache size.
/// @param address uint
/// @param cache_size int 
/// @param cache_name string
void print_index_and_tag(unsigned int address, int cache_size, const char* cache_name) {
    int index_bits = (int)log2(cache_size / BLOCK_SIZE);
    int index_mask = (1 << index_bits) - 1;
    int index = (address >> (int)log2(BLOCK_SIZE)) & index_mask;
    unsigned int tag = address >> (index_bits + (int)log2(BLOCK_SIZE));

    printf("%s Address %08X: Index = %d, Tag = %08X\n", cache_name, address, index, tag);
}

/// @brief Function to reconstruct the full address from index and tag stored in a cache line.
/// @param index Index of the cache line.
/// @param tag Tag stored in the cache line.
/// @param cache_size Size of the cache to determine the number of index bits.
/// @return The reconstructed full address.
unsigned int get_full_address(int index, unsigned int tag, int cache_size) {
    int index_bits = (int)log2(cache_size / BLOCK_SIZE);  // Calculate the number of index bits
    int offset_bits = (int)log2(BLOCK_SIZE);            // Calculate the number of offset bits

    unsigned int address = tag << (index_bits + offset_bits); // Shift tag left to make space for index and offset
    address |= (index << offset_bits);                        // Insert index into the correct position
    // Offset is assumed to be zero since we're usually dealing with entire blocks

    return address;
}
void moveToDram(unsigned int address){
    printf("moveToDram , %08x\n", address);
}
void hit_miss_finder(CacheLine *L1, CacheLine *L2, CacheLine *L3, unsigned int address)
{
    if (is_in_cache_L1(L1, address)==1)
    {
        printf("Hit on L1 for adress %08x\n", address);
        hits++;
        cycles += L1_cycels;
    }
    else if (is_in_cache_L2(L2, address)==1)
    {
        printf("Hit on L2 for adress %08x\n", address);
        hits++;
        cycles += L2_cycels;
    }
    else if (is_in_cache_L3(L3, address)==1)
    {
        printf("Hit on L3 for adress %08x\n", address);
        hits++;
        cycles += L3_cycels;
    }
    else
    {
        printf("Not found on cache. Upload form DRAM %08x\n", address);
        misses++;
        cycles += DRAM_Cycle;
    }
}


void LRU(CacheLine* L1, CacheLine* L2, CacheLine* L3, unsigned int address) {
    if(is_valid_bit_set(L1, address,L1_SIZE)==0) {//If L1 was never writen then write.
        update_cache_L1(L1, address);
        return;
    }
    else
    {
        if(is_in_cache_L1(L1,address))// If tag is match
            return;
        else{// If tag is missmatch take the L1 value that should be move to L2.
             int index = (address >> (int)log2(BLOCK_SIZE)) & ((1 << (int)log2(L1_SIZE / BLOCK_SIZE)) - 1);
             unsigned int storedTag = L1[index].tag;
             oldL1Address = get_full_address(index, storedTag, L1_SIZE);
             update_cache_L1(L1, address); // Update L1, now should handle the old L1.
             if (is_valid_bit_set(L2,oldL1Address,L2_SIZE)==0){// If valid is 0 for the old L1 address.
                 update_cache_L2(L2, oldL1Address);
             }
             else{// If the place who now belong to the old L1 is capture. 
             // Get the old L2 address and update L2.
                int index = (oldL1Address >> (int)log2(BLOCK_SIZE)) & ((1 << (int)log2(L2_SIZE / BLOCK_SIZE)) - 1);
                unsigned int storedTag = L2[index].tag;
                oldL2Address = get_full_address(index, storedTag, L2_SIZE);
                update_cache_L2(L2, oldL1Address);
                if(is_valid_bit_set(L3,oldL2Address,L3_SIZE)==0){//If valid is 0 so old L2 have place
                    update_cache_L3(L3, oldL2Address);
                }
                else{// L2 dont have place.
                     int index = (oldL2Address >> (int)log2(BLOCK_SIZE)) & ((1 << (int)log2(L3_SIZE / BLOCK_SIZE)) - 1);
                     unsigned int storedTag = L3[index].tag;
                     oldL3Address = get_full_address(index, storedTag, L3_SIZE);
                     update_cache_L3(L3, oldL2Address);
                    if(address!=oldL3Address)
                     moveToDram(oldL3Address);
                }
             }
            }
    }
}

    void full_catch_logic(CacheLine * L1, CacheLine * L2, CacheLine * L3, unsigned int address){
        hit_miss_finder(L1, L2, L3, address);
        LRU(L1, L2, L3, address);
    }

int main() {

    // Initialize caches
    CacheLine* L1 = initialize_cache(L1_SIZE);
    CacheLine* L2 = initialize_cache(L2_SIZE);
    CacheLine* L3 = initialize_cache(L3_SIZE);
     // Test addresses - designed to generate a mix of hits, misses, and evictions
  unsigned int test_addresses[] = {
    0x1A2B3C00,0x1A2B3C00,0x0A2B3C00,0x0A2B3C00,0x1A2B3C00,0x2A2B3C00,0x0A2B3C00,0x3A2B3C00,
     0x1A2B3C00,0x1A2B3C00,0x0A2B3C00,0x0A2B3C00,0x1A2B3C00,0x2A2B3C00,0x0A2B3C00,0x3A2B3C00
};



    for (int i = 0; i < sizeof(test_addresses) / sizeof(test_addresses[0]); i++) {
        printf("\nAccessing Address: %08X\n", test_addresses[i]);
      //  print_index_and_tag(test_addresses[i],L1_SIZE,"l1");
        full_catch_logic(L1, L2, L3, test_addresses[i]);
        //print_cache_values(L1, L1_SIZE, "L1");
        //print_cache_values(L2, L2_SIZE, "L2");
        //print_cache_values(L3, L3_SIZE, "L3");
        printf("Hits: %u, Misses: %u, Total Cycles: %u\n", hits, misses, cycles);
    }


return 0;
}