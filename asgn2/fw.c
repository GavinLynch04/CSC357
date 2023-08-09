/*Takes a file of words and sorts based on frequency, returning the
 *  top 10 (or how many the user requests) words from the file. 
 *  Uses a hash table and quadratic probing to achieve this
 *  while still maintaining quick runtimes.*/
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
typedef struct hash {
    char *data;   
    int numberOf;
} hash;
int num_words = 0;
int top_words = 10;
int table_size = 1000;
int word;
FILE* file;
char *line;
int value;
hash** globalMap;
hash* big;
int size = 100*sizeof(char);

void addElement(hash**, char*, int);

/* Allocates hash map based on size given*/
hash** createMap(int size) {
    int i;
    hash** make;
    if(!(make = (hash**)malloc(size * sizeof(hash)))) {
        perror("malloc");
        exit(EXIT_FAILURE);
    }
    for(i=0; i<size; i++) {
        make[i] = NULL;
    }
    return make;
}

/*Creates the key for each string given a char pointer*/
int hashValue(char *str) {
    unsigned long hash = 5381;
    int c;
    while((c = *str++)) {
        hash = ((hash << 5) + hash) + c;
    }
    return hash%table_size;
}

/*Resizes the hashmap when called by copying over elements*/
hash** resize(hash **map) {
    int j;
    hash** map2;
    table_size = table_size * 2;
    map2 = createMap(table_size);
    num_words = 0;
    for(j = 0; j < table_size/2; j++) {
        if(map[j] != NULL) {
            addElement(map2, map[j]->data, map[j]->numberOf);
	    free(map[j]);
	}
    }
    free(map);
    return map2;
}

/*Adds an element to the hash map, allocation memory and 
 * checking load factor. Also checks if already in map*/
void addElement(hash **map, char *str, int num) {
    int quad_value;
    int i;
    int inserted;
    hash* item;
    if(!(item = (hash*)malloc(sizeof(hash)))) {
        perror("malloc");
        exit(EXIT_FAILURE);
    }
    if((double)num_words/table_size > 0.5) {
	map = resize(map);
	globalMap = map;
    }
    value = hashValue(str);
    item->data = str;
    item->numberOf = num;
    inserted = 0;
    i = 0;
    while(!inserted) {
        quad_value = (int) (value + (i*i)) % table_size;
        quad_value = abs(quad_value);
	if(map[quad_value] != NULL &&
	!strcmp(map[quad_value]->data, item->data)) {
            map[quad_value]->numberOf++;
	    inserted = 1;
	    free(item->data);
	    free(item);
        } else if(map[quad_value] == NULL) {
            map[quad_value] = item;
	    inserted = 1;
	    num_words++;
        } i++;
    }
}

/*Gets the most common n elements in the hash map by iterating
 * through the map. Then prints them in a list.*/
void getMostCommon(hash **map) {
    int i;
    int j;
    int empty = 1;
    if(!(big = (hash *)malloc(sizeof(hash)))) {
                perror("malloc");
                exit(EXIT_FAILURE);
    }
    big->numberOf = 0;
    big->data = "aaaaaaaaaa";
    printf("%s %d %s %d%s\n", "The top", top_words, 
    "words (out of", num_words, ") are:");
    for(i = 0; i < top_words; i++) {
    	for(j = 0; j < table_size; j++) {
            if(map[j] != NULL) {
		if(big->numberOf < map[j]->numberOf) {
		    if(strcmp(big->data, "aaaaaaaaaa") == 0)
		         free(big);
	     	    big = map[j];
		    empty = 0;
		} else if(big->numberOf == map[j]->numberOf && strcmp(big->data,
		map[j]->data) < 0) {
		    big = map[j];
		}
	    }
	}
	if(big->numberOf != 0) {
	    printf("%9d %s\n",  big->numberOf, big->data);
            big->numberOf = 0;
        }
    }
    if(empty)
	free(big);
}

/*Reads a word from the given file, allocating memory for it as well.*/
char *readWord(FILE *file) {
        int c;
	int i = 0;
        c = fgetc(file);
        size = 10 * sizeof(char);
        if(!(line = (char *)malloc(size))) {
                perror("malloc");
                exit(EXIT_FAILURE);
        }
        while(isalpha(c)) {
                if(i == size-1) {
                        size += 10*sizeof(char);
                        if(!(line = (char *)realloc(line, size))) {
                                perror("realloc");
                                exit(EXIT_FAILURE);
                        }
                }
		c = tolower(c);
                line[i] = c;
                i++;
		c = fgetc(file);
        }
	if(i!=0) {
            line[i] = '\0';
	    return line;
	}
	free(line);
	return NULL;
}

/*Opens files and prints error messages*/
void main_helper(int argc, char const *argv[], char const *n) {
    int i;
    if(n != '\0') {
        i = 3;
	top_words = atoi(n);
    } else {
	i = 1;
    } for (;i < argc; i++) {
        if(!(file = fopen(argv[i], "r"))) {
            perror("Invalid file");
	} else if (file != NULL) {
            while(!feof(file)) {
                line = readWord(file);
                if(line != NULL) {
		     addElement(globalMap, line, 1);
                }
            }
	     fclose(file);
        } else {
	     printf("%s\n", "usage: fw [-n num][file1 [file2...]]");
        }
    }
}

/*Handles command line arguments, and calls other functions*/
int main(int argc, char const *argv[]) {
    int i;
    globalMap = createMap(table_size);
    if(argc == 1) {
        file = stdin;
	while(!feof(file)) {
            line = readWord(file);
            if(line != NULL) {
                 addElement(globalMap, line, 1);
            }
        }
    } else if (argc == 2) {
	if(strcmp(argv[1], "-n") != 0) {
	    main_helper(argc, argv, '\0');
	}else
	    printf("%s\n", "usage: fw [-n num][file1 [file2...]]");
    } else if (argc == 3) {
        if(strspn(argv[2], "0123456789") != strlen(argv[2]) &&
        strcmp(argv[1], "-n") != 0)
	    main_helper(argc, argv, '\0');
	else
	    printf("%s\n", "usage: fw [-n num][file1 [file2...]]");
    } else if (argc == 4) {
	if(strspn(argv[2], "0123456789") == strlen(argv[2]) && 
	strcmp(argv[1], "-n") == 0) {
	    main_helper(argc, argv, argv[2]);
        } else
	    main_helper(argc, argv, '\0');
    } else {
	if(strspn(argv[2], "0123456789") == strlen(argv[2]) && 
	strcmp(argv[1], "-n") == 0) {
	    main_helper(argc, argv, argv[2]);
	} else {
	    main_helper(argc, argv, '\0');
	}
    }
    getMostCommon(globalMap);
    for(i=0; i<table_size; i++) {
	if(globalMap[i] != NULL) {
            free(globalMap[i]->data);
	    free(globalMap[i]);
	}
    }
    free(globalMap);
    return 0;
}
