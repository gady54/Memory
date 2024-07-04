#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>



void main()
{
    char buffer[256];
    char info[256];
    FILE* source = fopen("coremark_val.trc", "r");
    if (!source) {
        perror("Failed to open configuration file");
        exit(EXIT_FAILURE);
    }
    // Open the file for writing ("w" mode)
    FILE* destination = fopen("coremark_val.txt", "w");
    if (destination == NULL) {
        perror("Error opening file");
        return EXIT_FAILURE;
    }

    while (fgets(buffer, sizeof(buffer), source)) {
        char* pos = strstr(buffer, "lw");
        char* pos2 = strstr(buffer, "sw");
        if (pos) {
            fprintf(destination, "%s", pos);
            fscanf(source, "%s", info);
            fprintf(destination, info);
        }
        else if (pos2)
        {
            fprintf(destination, "%s", pos2);

        }

    }