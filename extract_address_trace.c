#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define MAX_LINE_LENGTH 256
#define NUM_REGISTERS 32

typedef struct {
    char name[3];
    unsigned int address;
} Register;

void init_registers(Register registers[], int num_registers) {
    for (int i = 0; i < num_registers; i++) {
        registers[i].address = (unsigned int)rand() % 0xFFFFFFFF;
    }
}

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

void process_command(char *command, Register registers[], int num_registers) {
    char instruction[3];
    char reg1[3], reg2[3];
    int offset;

    if (sscanf(command, "%s %[^,],%d(%[^)])", instruction, reg1, &offset, reg2) == 4) {
        unsigned int base_address = get_register_address(registers, num_registers, reg2);
        unsigned int final_address = base_address + (unsigned int)offset;

        if (strcmp(instruction, "lw") == 0 || strcmp(instruction, "sw") == 0) {
            printf("Command: %s, Final address: 0x%08X, Register: %s\n", command, final_address, strcmp(instruction, "lw") == 0 ? reg1 : reg2);
        }

        // For 'sw' commands, update the base register address
        if (strcmp(instruction, "sw") == 0) {
            update_register_address(registers, num_registers, reg2, final_address);
        }

        // Simulate sending the address to the cache memory simulator
        // This would be an actual function call in a real application
        // send_to_cache_simulator(final_address);
    }
}

int main() {
    srand((unsigned int)time(NULL));

    Register registers[NUM_REGISTERS] = {
        {"ra", 0}, {"sp", 0}, {"a0", 0}, {"a1", 0},
        {"a2", 0}, {"a3", 0}, {"a4", 0}, {"a5", 0},
        {"a6", 0}, {"a7", 0}, {"t0", 0}, {"t1", 0},
        {"t2", 0}, {"t3", 0}, {"t4", 0}, {"t5", 0},
        {"t6", 0}, {"s0", 0}, {"s1", 0}, {"s2", 0},
        {"s3", 0}, {"s4", 0}, {"s5", 0}, {"s6", 0},
        {"s7", 0}, {"s8", 0}, {"s9", 0}, {"s10", 0},
        {"s11", 0}, {"x0", 0}, {"x1", 0}, {"x2", 0}
    };

    init_registers(registers, NUM_REGISTERS);
    print_registers(registers, NUM_REGISTERS);

    FILE *inputFile = fopen("linpack_val.txt", "r");
    if (inputFile == NULL) {
        perror("Error opening input file");
        return 1;
    }

    char line[MAX_LINE_LENGTH];
    while (fgets(line, sizeof(line), inputFile)) {
        if (line[strlen(line) - 1] == '\n') {
            line[strlen(line) - 1] = '\0';  // Remove newline character
        }
        process_command(line, registers, NUM_REGISTERS);
    }

    fclose(inputFile);

    return 0;
}
