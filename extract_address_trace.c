#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "extract_address_trace.h"

#define MAX_LINE_LENGTH 256

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

void send_to_cache_simulator(unsigned int address) {
    // Placeholder function to simulate sending the address to the cache memory simulator
    printf("Address 0x%08X sent to cache memory simulator\n", address);
}

unsigned int process_command(char *command, Register registers[], int num_registers) {
    char instruction[3];
    char reg1[3], reg2[3];
    int offset;

    if (sscanf(command, "%s %[^,],%d(%[^)])", instruction, reg1, &offset, reg2) == 4) {
        unsigned int base_address = get_register_address(registers, num_registers, reg2);
        unsigned int final_address = base_address + (unsigned int)offset;

        if (strcmp(instruction, "lw") == 0 || strcmp(instruction, "sw") == 0) {
            //printf("Command: %s, Final address: 0x%08X, Register: %s\n", command, final_address, strcmp(instruction, "lw") == 0 ? reg1 : reg2);
            send_to_cache_simulator(final_address);  // Send the address to the cache simulator
        }

        // For 'sw' commands, update the base register address
        if (strcmp(instruction, "sw") == 0) {
            update_register_address(registers, num_registers, reg2, final_address);
        }

        return final_address;
    }
    return 0;
}

int extract_addresses_from_file(const char *filename, unsigned int **addresses) {
    Register registers[NUM_REGISTERS] = {
        {"ra", 0x00000000}, {"sp", 0xd53a2000}, {"a0", 0x3000}, {"a1", 0x4000},
        {"a2", 0x5000}, {"a3", 0x6000}, {"a4", 0x7000}, {"a5", 0x8000},
        {"a6", 0x9000}, {"a7", 0xA000}, {"t0", 0xB000}, {"t1", 0xC000},
        {"t2", 0xD000}, {"t3", 0xE000}, {"t4", 0x569aF000}, {"t5", 0x10000},
        {"t6", 0x11000}, {"s0", 0x1238b000}, {"s1", 0x13000}, {"s2", 0x14000},
        {"s3", 0xbb815000}, {"s4", 0x16000}, {"s5", 0x17000}, {"s6", 0x18000},
        {"s7", 0x19000}, {"s8", 0xba61A000}, {"s9", 0x1B000}, {"s10", 0x1C000},
        {"s11", 0x1D000}, {"x0", 0x1E000}, {"x1", 0xff81F000}, {"x2", 0xfa620000}
    };

    //print_registers(registers, NUM_REGISTERS);  // Print initial register addresses for debugging

    FILE *inputFile = fopen(filename, "r");
    if (inputFile == NULL) {
        perror("Error opening input file");
        return 0;
    }

    char line[MAX_LINE_LENGTH];
    int count = 0;
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
