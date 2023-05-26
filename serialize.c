// RUNNING COMMAND: 
// ---> ./serialize /PATH/input.exe serialize/serialized_output.txt

#include <stdio.h>

void serializeFile(const char* inputFilePath, const char* outputFilePath) {
    FILE* inputFile = fopen(inputFilePath, "rb");
    FILE* outputFile = fopen(outputFilePath, "w");

    if (inputFile == NULL || outputFile == NULL) {
        printf("Error opening files.\n");
        return;
    }

    int byte;
    while ((byte = fgetc(inputFile)) != EOF) {
        fprintf(outputFile, "%02X ", byte); // Serialize byte as hexadecimal string
    }

    fclose(inputFile);
    fclose(outputFile);
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        printf("Usage: %s <inputFilePath> <outputFilePath>\n", argv[0]);
        return 1;
    }

    const char* inputFilePath = argv[1];
    const char* outputFilePath = argv[2];

    serializeFile(inputFilePath, outputFilePath);

    printf("File serialized successfully.\n");

    return 0;
}
