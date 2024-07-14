#include <stdio.h>
#include <string.h>

#define MAX_LINE_LENGTH 256
/*/
int main() {
    FILE *inputFile, *outputFile;
    char line[MAX_LINE_LENGTH];

    // Open the input file for reading
    inputFile = fopen("linpack_val.txt", "r");
    if (inputFile == NULL) {
        perror("Error opening input file");
        return 1;
    }

    // Open the output file for writing
    outputFile = fopen("linpack_val1.txt", "w");
    if (outputFile == NULL) {
        perror("Error opening output file");
        fclose(inputFile);
        return 1;
    }

    // Read each line from the input file and write it to the output file if it does not contain "Info"
    while (fgets(line, sizeof(line), inputFile)) {
        if (strstr(line, "Info") == NULL) {
            fputs(line, outputFile);
        }
    }

    // Close the files
    fclose(inputFile);
    fclose(outputFile);

    printf("Filtered lines have been written to output.txt\n");

    return 0;
}
/*/