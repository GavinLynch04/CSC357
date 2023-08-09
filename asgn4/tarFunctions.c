/*Set of functions that are used in tar, separated to keep clutter lower*/
#include <stdio.h>

#define NAME_SIZE 100
#define MODE_OFFSET 100
#define SIZE_OFFSET 124
#define MTIME_OFFSET 136
#define PREFIX_OFFSET 345
#define PREFIX_LENGTH 155
#define LINKNAME_OFFSET 157
#define FULLNAME 256
#define MUG_SIZE 8
#define TIME_SIZE 12

/*Gets name from a buffer*/
char* getName(char* header, char* name) {
    int i;
    int old;
    char placeholder[FULLNAME];
    memset(placeholder, '\0', FULLNAME);
    memset(name, '\0', FULLNAME);
    i = 0;
    while(header[i] != '\0' && i < NAME_SIZE) {
        name[i] = header[i];
        i++;
    }
    old = i;
    i = PREFIX_OFFSET;
    while(header[i] != '\0' && i < PREFIX_OFFSET+PREFIX_LENGTH){
        placeholder[i-PREFIX_OFFSET] = header[i];
        i++;
    }

    if(i == PREFIX_OFFSET) {
        i = old;
    } else if(i > PREFIX_OFFSET) {
        strcat(placeholder, "/");
        strcat(placeholder, name);
        strcpy(name, placeholder);
        return name;
    }
    name[i] = '\0';
    return name;
}

/*Gets the symlink name*/
char* getSymLink(char* header, char* symLink) {
    int i = LINKNAME_OFFSET;
    while(header[i] && i-LINKNAME_OFFSET < NAME_SIZE) {
        symLink[i-LINKNAME_OFFSET] = header[i];
        i++;
    }
    symLink[i] = '\0';
    return symLink; 
}

/*Gets the mode of a header*/
char* getMode(char* header, char* mode) {
    int i = MODE_OFFSET;
    while(i-MODE_OFFSET < MUG_SIZE) {
        mode[i-MODE_OFFSET] = header[i];
        i++;
    }
    mode[i] = '\0';
    return mode;
}
/*gets the size of a file*/
char* getSize(char* header, char* octalSize) {
    int i = SIZE_OFFSET;
    while(i-SIZE_OFFSET < TIME_SIZE) {
        octalSize[i-SIZE_OFFSET] = header[i];
        i++;
    }
    octalSize[i] = '\0';
    return octalSize;
}

