#ifndef tarFunctions
#define tarFunctions

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

char* getName(char*, char*);
char* getMode(char*, char*);
char* getSize(char*, char*);
char* getMTime(char*, char*);
char* getSymLink(char*, char*);

#endif
