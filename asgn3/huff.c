/*Program to create the huffman encodings for a given file,
 * then print them to standerd out*/
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdint.h>

/*Macros used in both files*/
#define CHAR_SIZE 256
#define BUFFER_SIZE 2048

/*Type used for tree and linked list*/
typedef struct Node {
    int freq;
    int c;
    struct Node *next;
    struct Node *right;
    struct Node *left;
} Node;

/*Type used for frequency collection and printing codes*/
typedef struct Tuple {
    int c;
    int freq;
    char* code;
} Tuple;

int listLen = 0;
int listSize = 10;
Node* insert(Node*, Node*);

/*Creates the initial linked list from the in order frequency list*/
Node* createLinked(Node* list, Tuple *element) {
    Node *newNode;
    if (!(newNode = (Node *)malloc(sizeof(Node)))) {
        perror("malloc");
        exit(EXIT_FAILURE);
    }
    newNode->freq = element->freq;
    newNode->c = element->c;
    newNode->next = NULL;
    newNode->right = NULL;
    newNode->left = NULL;
    if(listLen == 0) {
        list = newNode;
        listLen++;
    } else {
        newNode->next = list;
        list = newNode;
        listLen++;
    }
    return list;
}

/*Reads the built final tree, collects the codes and saves them to tuples*/
Tuple* readTree(Node* tree, Tuple* freqlist, int table[], int val) {
    char* code;
    int i;
    if(tree->left) {
        table[val] = 0;
        freqlist = readTree(tree->left, freqlist, table, val + 1);
    } if (tree->right) {
        table[val] = 1;
        freqlist = readTree(tree->right, freqlist, table, val + 1);
    } if (!(tree->left) && !(tree->right)) {
        if(!(code = malloc(sizeof(char)*(val + 1)))){
            perror("Malloc");
            exit(EXIT_FAILURE);
        }
        for (i = 0 ; i < val ; ++i) {
            code[i] = table[i] + '0';
        }
        code[i] = '\0';
        freqlist[tree->c].code = code;
        freqlist[tree->c].c = tree->c;
    }
    return freqlist;
}

/*Combines the finished linked list into the final tree by combinging nodes*/
Node* combineTree(Node* list) {
    int i;
    Node* newNode;
    for (i = 0; i < listLen-1; listLen--) {
        if (!(newNode = (Node *)malloc(sizeof(Node)))) {
            perror("malloc");
            exit(EXIT_FAILURE);
        }
        newNode->freq = list->freq + list->next->freq;
        newNode->left = list;
        newNode->right = list->next;
        newNode->next = NULL;
        list = list->next->next;
        list = insert(list, newNode);
    }
    return list;
}

/*Inserts items into the linked list*/
Node* insert(Node* list, Node* node) {
    Node* temp = list;
    int inserted = 0;
    if(list == NULL) {
        list = node;
    } else if (list->freq >= node->freq) {
        node->next = list;
        list = node;
        inserted = 1;
    } else {
        while(list != NULL && !inserted) {
            if(list->next == NULL) {
                list->next = node;
                inserted = 1;
            } else if(list->next->freq >= node->freq) {
                node->next = list->next;
                list->next = node;
                inserted = 1;
            } else
                list = list->next;
        }
        list = temp;
    }
    return list;
}

/*Reads a char from the file and saves it to the list*/
Tuple* readChar(Tuple* freqlist, unsigned char buffer[], ssize_t bytes) {
    int i;
    int c;
    for(i = 0; i < bytes; i++) {
        c = buffer[i];
        if(freqlist[c].c == -1) {
            freqlist[c].freq = 1;
            freqlist[c].c = c;
        } else {
            freqlist[c].freq++;
        }   
    }
    return freqlist;
}

/*Function to sort the frequency list based on freq and alpha order*/
int cmpfunc (const void * a, const void * b) {
    Tuple* a2 = (Tuple*) a;
    Tuple* b2 = (Tuple*) b;
    if(a2->freq < b2->freq)
        return 1;
    else if(a2->freq > b2->freq)
        return -1;
    else {
        if(a2->c < b2->c)
            return 1; 
        else if(a2->c > b2->c)
            return -1;
        else
            return 0;
    }
}

/*Function to sort the end list by alpha order*/
int cmpfunc2(const void * a, const void * b) {
    Tuple* a2 = (Tuple*) a;
    Tuple* b2 = (Tuple*) b;
    if(a2->c > b2->c)
        return 1;
    else if(a2->c < b2->c)
        return -1;
    else
        return 0; 
}

/*Frees entire tree*/
void freeTree(Node* tree) {
    if(tree == NULL)
        return;
    freeTree(tree->left);
    freeTree(tree->right);
    free(tree);
}

/*Frees freqlist and codes stored in it*/
void freeFreqList(Tuple* list) {
    int i;
    for(i = 0; i < CHAR_SIZE; i++) {
        if(list[i].code != NULL) {
            free(list[i].code);
        }
    }
    free(list);
}

