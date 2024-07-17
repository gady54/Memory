#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "extract_address_trace.h"

#define MAX_LINE_LENGTH 256
char instruction[5];
char reg1[3], reg2[3];
int offset;
unsigned int address1, address2;
int count = 0;

void print_registers(const Register registers[], int num_registers) {
    printf("Initial register addresses:\n");
    for (int i = 0; i < num_registers; i++) {
        printf("%s: 0x%08X\n", registers[i].name, registers[i].address);
    }
}

unsigned int get_register_address(const Register registers[], int num_registers, const char *name) {
    for (int i = 0; i < num_registers; i++) {
        if (strcmp(registers[i].name, name) == 0) {
            return registers[i].address;
        }
    }
    return 0;
}

void update_register_address(Register registers[], int num_registers, const char *name, unsigned int address) {
    for (int i = 0; i < num_registers; i++) {
        if (strcmp(registers[i].name, name) == 0) {
            registers[i].address = address;
            break;
        }
    }
}

void send_to_cache_simulator(const char *reg_name, unsigned int address) {
    printf("Register %s: Final address 0x%08X sent to cache memory simulator\n", reg_name, address);
}

unsigned int process_command(char *command, Register registers[], int num_registers) {
    if (sscanf(command, "Info %s %x -> %x", reg1, &address1, &address2) == 3) {
        update_register_address(registers, num_registers, reg1, address2);
        //send_to_cache_simulator(reg1, address2);
        count--;
        return address2;
    } else if (sscanf(command, "%s %[^,],%d(%[^)])", instruction, reg1, &offset, reg2) == 4) {
        unsigned int base_address = get_register_address(registers, num_registers, reg2);
        unsigned int final_address = base_address + (unsigned int)offset;

        if (strcmp(instruction, "lw") == 0 || strcmp(instruction, "sw") == 0) {
            //send_to_cache_simulator(strcmp(instruction, "lw") == 0 ? reg1 : reg2, final_address);// Send the address to the cache simulator
            update_register_address(registers, num_registers, reg1, final_address);  
        }

        // For 'sw' commands, update the base register address
        if (strcmp(instruction, "sw") == 0) {
            update_register_address(registers, num_registers, reg2, final_address);
        }

        return final_address;
    } else if (sscanf(command, "%s %[^,],%[^,],%d", instruction, reg1, reg2, &offset) == 4) {
        if (strcmp(instruction, "addi") == 0) {
            unsigned int src_address = get_register_address(registers, num_registers, reg2);
            unsigned int new_address = src_address + (unsigned int)offset;
            update_register_address(registers, num_registers, reg1, new_address);
            //send_to_cache_simulator(reg1, new_address);  // Send the new address to the cache simulator
            return new_address;
        }
    }
    return 0;
}

int extract_addresses_from_file(const char *filename, unsigned int **addresses) {
    Register registers[NUM_REGISTERS] = {
        {"ra", 0x00000000}, {"sp", 0xd53a2000}, {"a0", 0x3000598a}, {"a1", 0x4023adc9},
        {"a2", 0xffaac532}, {"a3", 0xff6584ff}, {"a4", 0x1258cddf}, {"a5", 0x89cc2235},
        {"a6", 0x13235eee}, {"a7", 0xabb12899}, {"t0", 0xaa232210}, {"t1", 0xc0035894},
        {"t2", 0xaacc2254}, {"t3", 0xefd6358c}, {"t4", 0x569aF000}, {"t5", 0x112568dd},
        {"t6", 0x110122dd}, {"s0", 0x1238b000}, {"s1", 0x13000987}, {"s2", 0x14000354},
        {"s3", 0xbb815000}, {"s4", 0x16389ddb}, {"s5", 0x1701056a}, {"s6", 0x18000dda},
        {"s7", 0x19985520}, {"s8", 0xba61A000}, {"s9", 0x1B0d0334}, {"s10", 0x1C0211fd},
        {"s11", 0x1D045ddf}, {"x0", 0x1E0ffd20}, {"x1", 0xff81F000}, {"x2", 0xfa620000}
    };

    //print_registers(registers, NUM_REGISTERS);  // Print initial register addresses for debugging

    FILE *inputFile = fopen(filename, "r");
    if (inputFile == NULL) {
        perror("Error opening input file");
        return 0;
    }

    char line[MAX_LINE_LENGTH];
    
    unsigned int *temp_addresses = (unsigned int *)malloc(sizeof(unsigned int) * MAX_REQUESTS);

    while (fgets(line, sizeof(line), inputFile)) {
        if (line[strlen(line) - 1] == '\n') {
            line[strlen(line) - 1] = '\0';  // Remove newline character
        }
        unsigned int address = process_command(line, registers, NUM_REGISTERS);
        if (address != 0) {
            temp_addresses[count++] = address;
        }
    }

    fclose(inputFile);

    *addresses = (unsigned int *)malloc(sizeof(unsigned int) * (size_t)count);
    memcpy(*addresses, temp_addresses, sizeof(unsigned int) * (size_t)count);
    free(temp_addresses);
    
    return count;
}
/*/
int main() {
    unsigned int *addresses;
    extract_addresses_from_file("address.txt", &addresses);
 
    return 0;
}
/*/