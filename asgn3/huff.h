#ifndef MY_HEADER_H
#define MY_HEADER_H

#define CHAR_SIZE 256
#define BUFFER_SIZE 2048

typedef struct Node {
    int freq;
    int c;
    struct Node *next;
    struct Node *right;
    struct Node *left;
} Node;

typedef struct Tuple {
    int c;
    int freq;
    char* code;
} Tuple;

void freeTree(Node*);
int cmpfunc(const void*, const void*);
int cmpfunc2(const void*, const void*);
Tuple* readChar(Tuple*, unsigned char*, ssize_t);
Node* combineTree(Node*);
void insert(Node*);
Tuple* readTree(Node*, Tuple*, int*, int);
Node* createLinked(Node*, Tuple*);
void freeFreqList(Tuple*);

#endif
