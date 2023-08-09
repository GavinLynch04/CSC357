/*Encodes and given file based on huffman encoding standards*/
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdint.h>
#include <arpa/inet.h>
#include <string.h>
#include "huff.h"

Tuple* freqlist;
Node* list;

uint8_t num = -1;

/*Creates and writes the header to the file based on freq information*/
void createHeader(int outFile) {
    int i;
    ssize_t ret;
    int j = 1;
    uint8_t buffer[BUFFER_SIZE];
    uint32_t n;
    buffer[0] = num;
    for (i = 0; i < CHAR_SIZE; i++) {
        if(freqlist[i].c != -1) {
            n = (uint32_t)freqlist[i].freq;
            buffer[j] = (uint8_t)freqlist[i].c;
            buffer[j+1] = (uint8_t)(n >> 24);
            buffer[j+2] = (uint8_t)(n >> 16);
            buffer[j+3] = (uint8_t)(n >> 8);
            buffer[j+4] = (uint8_t)n;  
            j+=5;
        }
    }
    ret = write(outFile, buffer, j);
    if (ret == -1) {
        perror("write");
        exit(EXIT_FAILURE);
    }
}

/*Writes individual bits to a file from a byte)*/
void writeBits(unsigned char* buff, int bufSize, int file) {
    int i;
    ssize_t ret;
    uint8_t* buffer;
    size_t numBytes = (bufSize + 7) / 8;
    if(!(buffer = malloc(numBytes))) {
        perror("malloc");
        exit(EXIT_FAILURE);
    }
    memset(buffer, 0, numBytes); 
    for (i = 0; i < bufSize; i++) {
        if (buff[i] == '1') {
            buffer[i/8] |= (1 << (7 - (i % 8)));
        }
    }
    ret = write(file, buffer, numBytes);
    if (ret == -1) {
        perror("write");
        exit(EXIT_FAILURE);
    }
    free(buffer);
}

/*Generates body of encoded file by rereading the input file and writing codes*/
void rereadFile(int inFile, int outFile) {
    int i;
    int j;
    char* code;
    ssize_t bytes;
    unsigned char buffer[BUFFER_SIZE/8];
    unsigned char codeBuffer[BUFFER_SIZE*2];
    int bufLoc = 0;
    lseek(inFile, 0, SEEK_SET);
    bytes = read(inFile, buffer, sizeof(buffer));
    while(bytes > 0) {
        if (bytes == -1) {
            perror("read");
            exit(EXIT_FAILURE);
        }
        for(i = 0; i < bytes; i++) {
            code = freqlist[buffer[i]].code;
            for (j = 0; j <= strlen(code); j++) {
                codeBuffer[j + bufLoc] = code[j];
            }
            bufLoc += j-1;
            if ((bufLoc%8 == 0 && bytes == BUFFER_SIZE/8) || 
            (bytes < BUFFER_SIZE/8 && i+1 >= bytes)) {
                writeBits(codeBuffer, bufLoc, outFile);
                memset(codeBuffer, 0, sizeof(codeBuffer));
                bufLoc = 0;
            } 
        }
        bytes = read(inFile, buffer, sizeof(buffer));
    }
    if(bufLoc != 0) {
       writeBits(codeBuffer, bufLoc, outFile); 
    }
}

/*Opens and error checks input files, while reading to a buffer*/
int openInFile(char const fileName[]) {
    ssize_t bytes;
    off_t size;
    int file;
    unsigned char buffer[BUFFER_SIZE];
    file = open(fileName, O_RDONLY | O_CREAT, 0644);
    if (file == -1) {
        perror("open22");
        exit(EXIT_FAILURE);
    }
    size = lseek(file, 0, SEEK_END);
    if(size == 0) {
        close(file);
        free(freqlist);
        return -1;
    }
    lseek(file, 0, SEEK_SET);
    bytes = read(file, buffer, sizeof(buffer));
    while(bytes > 0) {
        if (bytes == -1) {
            perror("read");
            exit(EXIT_FAILURE);
        }
        freqlist = readChar(freqlist, buffer, bytes);
        bytes = read(file, buffer, sizeof(buffer));
    }
    return file;
}

/*Opens and error checks output files*/
int openOutFile(char const fileName[]) {
    int file;
    file = open(fileName, O_WRONLY | O_TRUNC | O_CREAT, 0644);
    if (file == -1) {
        perror("open");
        exit(EXIT_FAILURE);
    }
    return file; 
}

/*Handles general commandline input, and calling of other functions*/
int main(int argc, char const *argv[]) {
    int i;
    int table[CHAR_SIZE];
    int file;
    int outFile = 1;
    int val = 0;
    if(!(freqlist = malloc(sizeof(Tuple) * CHAR_SIZE))) {
        perror("malloc");
        exit(EXIT_FAILURE);
    }
    for(i = 0; i < CHAR_SIZE; i++) {
        freqlist[i].c = -1;
        freqlist[i].freq = 0;
    }
    if (argc == 2) {
        file = openInFile(argv[1]);
        if(file == -1)
            return 0;
    } else if(argc == 3) {
        file = openInFile(argv[1]);
        outFile = openOutFile(argv[2]);
        if(file == -1)
            return 0;
    } else {
        printf("%s", "Usage ./hencode inFile outFile");
        free(freqlist);
        return 0;
    }
   for(i = 0; i < CHAR_SIZE; i++) {
        if(freqlist[i].c != -1) {
            num++;
        }
    } 
    qsort(freqlist, CHAR_SIZE, sizeof(*freqlist), cmpfunc2);
    createHeader(outFile); 
    qsort(freqlist, CHAR_SIZE, sizeof(*freqlist), cmpfunc);
    for(i = 0; i < CHAR_SIZE; i++) {
        if(freqlist[i].c != -1) {
            num++;
            list = createLinked(list, &freqlist[i]);
        }
    }
    list = combineTree(list);
    for(i = 0; i < CHAR_SIZE; i++) {
        freqlist[i].c = -1;
        freqlist[i].code = '\0';
    }
    freqlist = readTree(list, freqlist, table, val);
    rereadFile(file, outFile); 
    freeFreqList(freqlist);
    freeTree(list);
    close(file);
    close(outFile);
    return 0;
}
