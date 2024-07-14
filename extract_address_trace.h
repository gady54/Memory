#ifndef EXTRACT_ADDRESS_TRACE_H
#define EXTRACT_ADDRESS_TRACE_H

#include <stdint.h>

#define MAX_REQUESTS 1000000
#define NUM_REGISTERS 32

typedef struct {
    char name[3];
    unsigned int address;
} Register;

unsigned int process_command(char *command, Register registers[], int num_registers);
int extract_addresses_from_file(const char *filename, unsigned int **addresses);

#endif // EXTRACT_ADDRESS_TRACE_H
