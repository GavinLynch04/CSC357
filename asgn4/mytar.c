/*Program that imitates the functions of tar, with compression, listing, 
 * extraction and various flag support*/
#include"tarFunctions.h"
#include<stdio.h>
#include<stdlib.h>
#include<getopt.h>
#include<utime.h>
#include<tar.h>
#include<grp.h>
#include<pwd.h>
#include<sys/stat.h>
#include<sys/types.h>
#include<fcntl.h>
#include<unistd.h>
#include<string.h>
#include<time.h>
#include<dirent.h>
#include<sys/sysmacros.h>

#define HEADER_SIZE 512
#define NAME__SIZE 100
#define MODE_OFFSET 100
#define TIMESIZE_SIZE 12
#define MUG_SIZE 8
#define USERGROUP_SIZE 32
#define UID_OFFSET 108
#define GID_OFFSET 116
#define SIZE_OFFSET 124
#define CHECKSUM_OFFSET 148
#define TYPE_OFFSET 156
#define LINK_OFFSET 157
#define MAGIC_OFFSET 257
#define VERSION_OFFSET 263
#define USER_OFFSET 265
#define GROUP_OFFSET 297
#define DEVMINOR_OFFSET 337
#define DEVMAJOR_OFFSET 329
#define MTIME_OFFSET 136
#define NAME_SIZE 256
#define MAX_FILENAMES 100
#define MAX_LIST 100
#define PREFIX_OFFSET 345
#define NAMES_SIZE 17
#define PER_SIZE 10

void readTarVerbose(char*);
int isEndOfFile(char*, int);
void headerCreate(char*, char*, struct stat*); 
int checkSum(char*);

int headCounter = 0;
int verbose = 0;
int strict = 0;

/*Creates formating for a spillover into prefix*/
void splitFilenameAtMaxLength(char *filename, char* header) {
    int i;
    int nameSize = 100;
    int len = strlen(filename);
    for (i = 0; i < len; i++) {
        if (filename[i] == '/') {
            int remainingLength = len - i - 1;
            if (remainingLength <= nameSize) {
                strncpy(&header[PREFIX_OFFSET], filename, i + 1);
                header[i+PREFIX_OFFSET] = '\0';
                strncpy(header, filename+i+1, strlen(filename+i+1));
                header[strlen(filename+1+i)] = '\0';
                return;
            }
        }
    }
}

/*Function for creating the headers for each individual file*/
void headerCreate(char* header, char* filename, struct stat* fileStat) {
    ssize_t linkLength;
    struct passwd *pwd;
    struct group *grp;
    int i;
    unsigned int checksum;
    memset(header, '\0', HEADER_SIZE);
    
    if(strlen(filename) > NAME__SIZE) {
        splitFilenameAtMaxLength(filename, header);
    } else {
        snprintf(header, NAME__SIZE+1, "%s", filename);
    }
    snprintf(header + MODE_OFFSET, MUG_SIZE, "%07o", 
    (unsigned int)(fileStat->st_mode & 07777));

    snprintf(header + UID_OFFSET, MUG_SIZE, "%07o", 
    (unsigned int)fileStat->st_uid);

    snprintf(header + GID_OFFSET, MUG_SIZE, "%07o", 
    (unsigned int)fileStat->st_gid);

    if(S_ISDIR(fileStat->st_mode)) {
        snprintf(header + SIZE_OFFSET, TIMESIZE_SIZE, "%d", 
        0);
        if(strlen(filename) < NAME__SIZE)
            snprintf(header, NAME__SIZE, "%s%s", filename, "/");
        else {
            header[strlen(header)] = '/';
            header[strlen(header)] = '\0';
        }
    } else {
        snprintf(header + SIZE_OFFSET, TIMESIZE_SIZE, "%011llo", 
        (unsigned long)fileStat->st_size);
    }
    snprintf(header + MTIME_OFFSET, TIMESIZE_SIZE, "%011lo", 
    (unsigned long)fileStat->st_mtime);
    memset(header + CHECKSUM_OFFSET, ' ', MUG_SIZE);
    if (S_ISREG(fileStat->st_mode)) {
        header[TYPE_OFFSET] = '0';
    } else if (S_ISDIR(fileStat->st_mode)) {
        header[TYPE_OFFSET] = '5';
    } else if (S_ISLNK(fileStat->st_mode)) {
        header[TYPE_OFFSET] = '2';
        linkLength = readlink(filename, header + LINK_OFFSET, 
        sizeof(header) - LINK_OFFSET);
        if (linkLength == -1) {
            perror("readlink");
            return;
        }
    } else {
        fprintf(stderr, "Unsupported file, continuing...");
        return;
    }
    snprintf(header + VERSION_OFFSET, 3, "00");
    snprintf(header + MAGIC_OFFSET, 6, "ustar");

    pwd = getpwuid(fileStat->st_uid);
    if (pwd == NULL) {
        perror("getpwuid");
        return;
    }
    grp = getgrgid(fileStat->st_gid);
    if (grp == NULL) {
        perror("getgrgid");
        return;
    }
    snprintf(header + USER_OFFSET, USERGROUP_SIZE, pwd->pw_name);
    snprintf(header + GROUP_OFFSET, USERGROUP_SIZE, grp->gr_name);

    snprintf(header + DEVMINOR_OFFSET, MUG_SIZE, "%u", minor(fileStat->st_dev));
    snprintf(header + DEVMAJOR_OFFSET, MUG_SIZE, "%u", major(fileStat->st_dev));

    checksum = 0;
    for (i = 0; i < HEADER_SIZE; i++) {
        checksum += (unsigned char)header[i];
    }
    snprintf(header + CHECKSUM_OFFSET, MUG_SIZE, "%06o", checksum);
}

/*creates tar files*/
void createTarFile(int tar, char* filename) {
    ssize_t bytes;
    DIR *dir;
    int input;
    char header[HEADER_SIZE];
    struct stat fileInfo;
    struct dirent *entry;
    if (lstat(filename, &fileInfo) == -1) {
            fprintf(stderr, "Failed to get file information for %s\n", 
            filename);
            return;
    }
    if (S_ISDIR(fileInfo.st_mode)) {
        headerCreate(header, filename, &fileInfo);
        if((write(tar, header, HEADER_SIZE)) == -1) {
            /*added error*/
        }
        if(!(dir = opendir(filename))) {
            perror("open");
            exit(EXIT_FAILURE);
        }
        if(verbose)
            printf("%s\n", filename);

        while ((entry = readdir(dir)) != NULL) {
            if (strcmp(entry->d_name, ".") != 0 && 
            strcmp(entry->d_name, "..") != 0) {
                filename = strcat(filename, "/");
                filename = strcat(filename, entry->d_name);
                createTarFile(tar, filename);
                filename[strlen(filename) - strlen(entry->d_name)-1] = '\0';
            }
        }
        closedir(dir);
    } else if (S_ISREG(fileInfo.st_mode) || S_ISLNK(fileInfo.st_mode)) {
        headerCreate(header, filename, &fileInfo);
        if((write(tar, header, HEADER_SIZE)) == -1) {
            fprintf(stderr, "Failed to write to tar file");
            return;
        }
        memset(header, '\0', HEADER_SIZE);
        
        input = open(filename, O_RDONLY);
        if (input == -1) {
            fprintf(stderr, "Failed to open input file %s for writing", 
            filename);
            return;
        }
        if(verbose)
            printf("%s\n", filename);
        while ((bytes = read(input, header, HEADER_SIZE)) > 0) {
            if((write(tar, header, HEADER_SIZE)) == -1) {
                fprintf(stderr, "Failed to write to tar file");
                return;
            }
            memset(header, '\0', HEADER_SIZE);
            if (bytes < HEADER_SIZE) {
                break;
            }
        }
        close(input);
    } else {
        fprintf(stderr, "File %s unsupported, continuing...\n", filename);
    }
}

/* Splits the file path into lower directories*/
char** splitFilePath(const char* filepath, int* count) {
    char** directories;
    int i;
    char* directory;

    char* path = strdup(filepath);
    char* token = strtok(path, "/");
    
    while (token != NULL) {
        (*count)++;
        token = strtok(NULL, "/");
    }

    directories = (char**)malloc((*count) * sizeof(char*));

    strcpy(path, filepath);
    token = strtok(path, "/");
    
    i = 0;
    while (token != NULL) {
        directory = strdup(token);
        if (i > 0) {
            directories[i] = (char*)malloc(strlen(directories[i - 1]) + 
            strlen(directory) + 2);
            sprintf(directories[i], "%s/%s", directories[i - 1], directory);
        } else {
            directories[i] = strdup(directory);
        }
        token = strtok(NULL, "/");
        i++;
    }
    free(path);
    free(token);
    free(directory);
    return directories;
}

/*extracts tar files into pwd*/
void extractTarFiles(char* file, char** filenames, int filenameLen) { 
    ssize_t bytes;
    char type;
    int i;
    int j;
    int contains;
    int user;
    int group;
    int others;
    int nextHeader;
    long mtime;
    long size;
    long store;
    char** directories;
    char mode[MUG_SIZE+1];
    char octalSize[TIMESIZE_SIZE+1];
    char octalTime[TIMESIZE_SIZE+1];
    char symLink[NAME__SIZE+1];
    char buffer[HEADER_SIZE];
    char name[NAME_SIZE];
    int newFile;
    mode_t permission;
    size_t len;
    struct utimbuf times;
    int octal = 8;
    int modeInt = 3;
    char* ustar = "ustar";
    int count = 0;
    int tar = open(file, O_RDONLY);
    if (tar == -1) {
        fprintf(stderr, "Open error");
        exit(EXIT_FAILURE);
    }
    while ((bytes = read(tar, buffer, HEADER_SIZE)) > 0) {
        contains = 0;
        if ((tar = isEndOfFile(buffer, tar)) == -1) {
            return;
        }
        if(strict) {
            if (checkStrict(buffer) == -1) {
                fprintf(stderr, "Bad header, exiting...");
                exit(EXIT_FAILURE); 
            }
        } else {
            if(strncmp(&buffer[MAGIC_OFFSET], ustar, 5) != 0 
            && checkSum(buffer)) {
                fprintf(stderr, "Bad header, exiting...");
                exit(EXIT_FAILURE);
            }
        }

        /*Extracts file name*/
        getName(buffer, name);
        i = 0;
        count = 0;
        if(filenameLen > 0) {
            while (i < filenameLen) {
                if(strncmp(filenames[i], name, strlen(filenames[i])) == 0) {
                    directories = splitFilePath(name, &count);
                    for(i = 0; i < count-1; i++) {
                       mkdir(directories[i], 0777);
                    }
                    contains = 1;
                    for (j = 0; j < count; j++) {
                        free(directories[j]);
                    }
                    free(directories);
                    break;
                }
                i++;
            }
            if(contains == 0) {
                getSize(buffer, octalSize);
                size = strtol(octalSize, NULL, octal);
                if(size%HEADER_SIZE != 0) {
                    size = size/HEADER_SIZE;
                    size++;
                } else 
                    size = size/HEADER_SIZE;
                size *= HEADER_SIZE;
                nextHeader = size;
                lseek(tar, nextHeader, SEEK_CUR);
                continue;
            }
        }
        if(verbose)
            printf("%s\n", name);
        
        /*Extracts file permissions*/
        getMode(buffer, mode);
        len = strlen(mode);
        if (len > modeInt) {
            memmove(mode, mode + len - modeInt, modeInt+1);
            mode[modeInt] = '\0';
        }
        permission = strtol(mode, NULL, octal);

        user = (permission >> 6) & 0x1;
        group = (permission >> 3) & 0x1;
        others = permission & 0x1;
        if (user || group || others) {
            permission = 0777; 
        } else {
            permission = 0666;
        }
 
        /*Determines type of file*/
        type = buffer[TYPE_OFFSET];
        if (type == '5') {
            newFile = mkdir(name, 0777);
        } else if (type == '2') {/*finish smylink*/
            getSymLink(buffer, symLink);
            symlink(name, symLink);

            i = MTIME_OFFSET;
            while(i-MTIME_OFFSET < TIMESIZE_SIZE) {
                octalTime[i-MTIME_OFFSET] = buffer[i];
                i++;
            }
            octalTime[i] = '\0';
            
            mtime = strtol(octalTime, NULL, MUG_SIZE);
            times.actime = time(NULL);
            times.modtime = (int)mtime;
            utime(name, &times);

        } else if (type == '\0' || type == '0') {
            newFile = open(name, O_CREAT | O_WRONLY | O_TRUNC, permission);
            if (newFile == -1) {
                perror("Open");
                break;
            }

            i = MTIME_OFFSET;
            while(i-MTIME_OFFSET < MUG_SIZE) {
                octalTime[i-MTIME_OFFSET] = buffer[i];
                i++;
            }
            octalTime[i] = '\0';
 
            mtime = strtol(octalTime, NULL, MUG_SIZE);
            times.actime = time(NULL);
            times.modtime = (int)mtime;
            utime(name, &times);

            getSize(buffer, octalSize);
            size = strtol(octalSize, NULL, MUG_SIZE);
            store = size;
            if(size%HEADER_SIZE != 0) {
                size = size/HEADER_SIZE;
                size++;
            } else 
                size = size/HEADER_SIZE;
            for(i = 0; i < size; i++) {
                bytes = read(tar, buffer, HEADER_SIZE);
                store -= bytes;
                if(store >= 0)
                    bytes = write(newFile, buffer, bytes);
                else
                   bytes = write(newFile, buffer, bytes+store);
            }
            close(newFile);
        } else 
            printf("%s %s %s", "Could not extract", name, 
            ", file type not supported");
    }
}

/* checks if end of file*/
int isEndOfFile(char* array, int tar) {
    int i;
    for (i = 0; i < HEADER_SIZE; i++) {
        if (array[i] != '\0') {
            return tar;
        }
    }
    read(tar, array, HEADER_SIZE);
    for (i = 0; i < HEADER_SIZE; i++) {
        if (array[i] != '\0') {
            lseek(tar, -HEADER_SIZE, SEEK_CUR);
            return tar;
        }
    }
    close(tar);
    return -1;
}

/*checks that it complies with strict standards*/
int checkStrict(char* buffer) {
    char* ustar = "ustar";
    if(strcmp(ustar, &buffer[MAGIC_OFFSET]) == 0 && 
    buffer[MAGIC_OFFSET+strlen(ustar)] == '\0' &&
    buffer[VERSION_OFFSET] == '0' && buffer[VERSION_OFFSET+1] == '0'
    && checkSum(buffer)) {
        return 1;
    } else {
        return -1;
    }
}

/*calculates the checksum of a file*/
int checkSum(char* buffer) {
    int i;
    char storedChecksum[MUG_SIZE+1];
    int checksum = 0;
    int checksumSize = 8;
    unsigned int storedChecksumDecimal = 0;
    for (i = 0; i < CHECKSUM_OFFSET; i++) {
        checksum += (unsigned char)buffer[i];
    }

    for (i = CHECKSUM_OFFSET + checksumSize; i < HEADER_SIZE; i++) {
        checksum += (unsigned char)buffer[i];
    }
    checksum += ' ' * checksumSize;

    memcpy(storedChecksum, &buffer[CHECKSUM_OFFSET], checksumSize);
    storedChecksum[checksumSize] = '\0';
    sscanf(storedChecksum, "%o", &storedChecksumDecimal);
    
    if(storedChecksumDecimal == checksum) {
        return 1;
    } else {
        return 0;
    }
}

/*reads the tar file to stdout*/
void readTar(char* file, char** filenames, int filenameLen) {
    ssize_t bytes;
    int nextHeader;
    long size;
    char* endptr;
    int i;
    char octalSize[TIMESIZE_SIZE+1];
    char name[NAME_SIZE];
    char buffer[HEADER_SIZE];
    char* ustar = "ustar";
    int tar = open(file, O_RDONLY);
    if (tar == -1) {
        perror("open");
        exit(EXIT_FAILURE);
    }
    while ((bytes = read(tar, buffer, HEADER_SIZE)) > 0) {
        if ((tar = isEndOfFile(buffer, tar)) == -1) {
            return;
        }
        if(strict) {
            if (checkStrict(buffer) == -1) {
                fprintf(stderr, "Bad header, exiting...");
                exit(EXIT_FAILURE); 
            }
        } else {
            if(strncmp(&buffer[MAGIC_OFFSET], ustar, 5) != 0 
            && checkSum(buffer)) {
                fprintf(stderr, "Bad header, exiting...");
                exit(EXIT_FAILURE);
            }
        }
        i = 0;
        getName(buffer, name);
        if(filenameLen > 0) {
            while (i < filenameLen) {
                if(strncmp(filenames[i], name, strlen(filenames[i])) == 0) {
                    if(verbose)
                        readTarVerbose(buffer);
                    /*Gets name from header*/
                    printf("%s\n", name);
                }
                i++;
            }
        } else {
            if(verbose)
                readTarVerbose(buffer);
            /*Gets name from header*/
            printf("%s\n", name);
        }
 
        /*Gets octal size*/
        getSize(buffer, octalSize);
        endptr = NULL;
        size = strtol(octalSize, &endptr, 8);
        if (endptr == octalSize) {
            fprintf(stderr, "Conversion error: invalid size\n");
            return;
        }
        if (size%HEADER_SIZE != 0) {
            size = size/HEADER_SIZE;
            size++;
        } else {
            size = size/HEADER_SIZE;
        }
        size *= HEADER_SIZE;
        nextHeader = size;
        lseek(tar, nextHeader, SEEK_CUR);
    }
}

/*Reads the verbose features for a file*/
void readTarVerbose(char* buffer) {
    char type;
    int i;
    char permissions[PER_SIZE+1];
    char names[NAMES_SIZE+1];
    char mode[MUG_SIZE+1];
    char octalTime[TIMESIZE_SIZE+1];
    char formattedTime[NAMES_SIZE];
    long size;
    int numDigits;
    long mtime;
    long numMode;
    char* endptr;
    struct tm* timeInfo;
    int loc;
    char octalSize[TIMESIZE_SIZE+1];
    int octal = 8;
    type = buffer[TYPE_OFFSET];
    if (type == '5') 
        permissions[0] = 'd';
    else if (type == '2')
        permissions[0] = 'l';
    else
        permissions[0] = '-';
    
    /*Calculates the permission portion of the table*/
    getMode(buffer, mode);
    
    endptr = NULL;
    numMode = strtol(mode, &endptr, octal);
    if (endptr == mode) {
        fprintf(stderr, "Conversion error: invalid mode\n");
        return;
    }
    permissions[1] = (numMode & 0400) ? 'r' : '-';
    permissions[2] = (numMode & 0200) ? 'w' : '-';
    permissions[3] = (numMode & 0100) ? 'x' : '-';
    permissions[4] = (numMode & 040) ? 'r' : '-';
    permissions[5] = (numMode & 020) ? 'w' : '-';
    permissions[4] = (numMode & 040) ? 'r' : '-';
    permissions[5] = (numMode & 020) ? 'w' : '-';
    permissions[6] = (numMode & 010) ? 'x' : '-';
    permissions[7] = (numMode & 04) ? 'r' : '-';
    permissions[8] = (numMode & 02) ? 'w' : '-';
    permissions[9] = (numMode & 01) ? 'x' : '-';
    permissions[10] = '\0';
    printf("%s ", permissions);

    /*Gets user name and group name*/
    i = USER_OFFSET;
    while(buffer[i] && i-USER_OFFSET < USERGROUP_SIZE) {
        names[i-USER_OFFSET] = buffer[i];
        i++;
    }
    names[i-USER_OFFSET] = '/';
    loc = i-USER_OFFSET+1;
    i = GROUP_OFFSET;
    while(buffer[i] && i-GROUP_OFFSET < USERGROUP_SIZE) {
        names[loc] = buffer[i];
        i++;
        loc++;
    }
    names[loc] = '\0';
    printf("%s ", names);

    /*Calculates the size portion of the table*/
    getSize(buffer, octalSize);
    endptr = NULL;
    size = strtol(octalSize, &endptr, octal);
    if (endptr == octalSize) {
        fprintf(stderr, "Conversion error: invalid size\n");
        return;
    }
    numDigits = snprintf(NULL, 0, "%ld", size);
    printf("%*s%ld ", octal-numDigits, "", size);

    /*Calculates the mtime from the octal*/
    i = MTIME_OFFSET;
 
    while(i-MTIME_OFFSET < TIMESIZE_SIZE) {
        octalTime[i-MTIME_OFFSET] = buffer[i];
        i++;
    }
    octalTime[i] = '\0';
    endptr = NULL; 
    mtime = strtol(octalTime, &endptr, octal);
    if (endptr == octalTime) {
        fprintf(stderr, "Conversion error: invalid time\n");
        return;
    }
 
    timeInfo = localtime(&mtime);
    strftime(formattedTime, sizeof(formattedTime), "%Y-%m-%d %H:%M", timeInfo);
    printf("%s ", formattedTime);
}

/*Handles options on the commandline and parsing, plus function calls*/
int main(int argc, char *argv[]) {
    int i;
    int j;
    int filenameLen;
    int optind;
    int tarFile;
    char operation;
    char header[HEADER_SIZE];
    char *filenames[MAX_FILENAMES];/*change to malloc*/
    char *inputFile = NULL;

    j = 0; 
    filenameLen = 0;
    if(argc == 1) {
        fprintf(stderr, "Usage mytar [ctxvS]f tarfile [path [...]]");
        exit(EXIT_FAILURE);
    }
    while  (j < strlen(argv[1])) {
        switch (argv[1][j]) {
            case 'c':
                operation = argv[1][j];
                break;
            case 't':
                operation = argv[1][j];
                break;
            case 'x':
                operation = argv[1][j];
                break;
            case 'f':
                i = 0;
                inputFile = argv[2];
                optind = 3;
                if(optind < argc) {
                    while (optind < argc) {
                        filenames[i] = argv[optind];
                        filenameLen++;
                        optind++;
                        i++;
                    }
                    filenames[i] = NULL;
                }
                break;
            case 'v':
                verbose = 1;
                break;
            case 'S':
                strict = 1;
                break;
            case '?':
                break;
            default:
                break;
        }
        j++;/*change to not i*/
    } if (operation == 't') {
        readTar(inputFile, filenames, filenameLen);
    } else if (operation == 'x') {
        extractTarFiles(inputFile, filenames, filenameLen);
    } else if (operation == 'c') {
        tarFile = open(inputFile, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (tarFile == -1) {
            fprintf(stderr, "Failed to open tar file for writing");
            return 0;
        }
        i = 0;
        while(filenames[i] != NULL) {
            createTarFile(tarFile, filenames[i++]);
        }
        memset(header, '\0', HEADER_SIZE);
        write(tarFile, header, HEADER_SIZE);
        memset(header, '\0', HEADER_SIZE);/*error check all*/
        write(tarFile, header, HEADER_SIZE);
        close(tarFile);
    } else
       fprintf(stderr, "Usage mytar [ctxvS]f tarfile [path [...]]"); 
    return 0;
}
