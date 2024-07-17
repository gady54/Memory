#ifndef CACHE_SIMULATION_H
#define CACHE_SIMULATION_H

#define L1_SIZE (32 * 1024) // 16KB
#define L2_SIZE (32 * 1024) // 32KB
#define L3_SIZE (1024 * 1024 * 2) // 2MB
#define BLOCK_SIZE 64 // Assuming block size is 64 bytes
#define ADDRESS_BITS 32 // Assuming a 32-bit address space

#define L1_cycles 1 // L1 access time in cycles
#define L2_cycles 6 // L2 access time in cycles
#define L3_cycles 30 // L3 access time in cycles

typedef struct {
    int valid; // Valid bit
    unsigned int tag; // Tag
} CacheLine;

CacheLine* initialize_cache(int cache_size);
void update_cache(CacheLine* cache, unsigned int address, int cache_size);
void reset_cache(CacheLine* cache, unsigned int address, int cache_size);
void update_cache_L1(CacheLine* L1, unsigned int address);
void update_cache_L2(CacheLine* L2, unsigned int address);
void update_cache_L3(CacheLine* L3, unsigned int address);
int is_in_cache(CacheLine* cache, unsigned int address, int cache_size);
int is_valid_bit_set(CacheLine* cache, unsigned int address, int cache_size);
int is_in_cache_L1(CacheLine* L1, unsigned int address);
int is_in_cache_L2(CacheLine* L2, unsigned int address);
int is_in_cache_L3(CacheLine* L3, unsigned int address);
void print_cache_values(CacheLine* cache, int cache_size, const char* cache_name);
void print_cache_info(int cache_size, const char* cache_name);
void print_index_and_tag(unsigned int address, int cache_size, const char* cache_name);
unsigned int get_full_address(unsigned int index, unsigned int tag, int cache_size);
void moveToDram(unsigned int address);
void hit_miss_finder(CacheLine* L1, CacheLine* L2, CacheLine* L3, unsigned int address);
void LRU(CacheLine* L1, CacheLine* L2, CacheLine* L3, unsigned int address);
void full_cache_logic(CacheLine* L1, CacheLine* L2, CacheLine* L3, unsigned int address);
void print_simulation_results();

unsigned int get_total_cycles();
unsigned int get_hits();
unsigned int get_misses();
unsigned int get_total_commands();

#endif // CACHE_SIMULATION_H
