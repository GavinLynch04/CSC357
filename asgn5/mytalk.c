/*Program to talk between two computers without blocking, 
 * allowing messages to send and recieve simultaneously*/
#include "talk.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <ctype.h>
#include <string.h>
#include <poll.h>

#define BUFFER_SIZE 512
#define STRING_SIZE 100

int dontAsk = 0;
int verbose = 0;
int noNcurses = 0;

/*Converts strings to lowercase*/
void convertToLowercase(char *str) {
    int i;
    if (str == NULL) {
        return;  // Handle null pointer
    }

    for (i = 0; str[i] != '\0'; i++) {
        str[i] = tolower((unsigned char)str[i]);
    }
}

/*Handle the sending and recieving of messages*/
void writeRead(int clientSocket) {
    struct pollfd files[2];
    char buffer[BUFFER_SIZE];
    int bytes;

    memset(buffer, 0, BUFFER_SIZE);

 
    files[0].fd = clientSocket;
    files[0].events = POLLIN;
    files[1].fd = fileno(stdin);
    files[1].events = POLLIN;
 
    while(1) {
        if ((poll(files, 2, -1)) == -1) {
            perror("poll");
            exit(EXIT_FAILURE);
        }
        if (files[0].revents & POLLIN) {
            bytes = recv(clientSocket, buffer, BUFFER_SIZE - 1, 0);
            if (bytes == -1) {
                perror("recv");
                exit(EXIT_FAILURE);
            } else if (bytes == 0) {
                write_to_output("Connection closed. ^C to terminate.", 35);
                while(getc(stdin) != EOF) {/*nothing*/}
                break;
            }
            write_to_output(buffer, strlen(buffer));
            memset(buffer, 0, BUFFER_SIZE);
        } else if (files[1].revents & POLLIN) {
            update_input_buffer();
            if(has_whole_line() && !has_hit_eof()) {
                bytes = read_from_input(buffer, BUFFER_SIZE-1);
                if (bytes == -1) {
                    perror("read");
                    exit(EXIT_FAILURE);
                }
                bytes = send(clientSocket, buffer, strlen(buffer), 0);
                if(bytes == -1) {
                    perror("send");
                    close(clientSocket);
                    exit(EXIT_FAILURE);
                }
                memset(buffer, 0, BUFFER_SIZE);
            } else if(has_hit_eof()) {
                break;
            }
        }
    }
} 

/*Handles the actions of the server*/
void serverActions(int port) {
    int backlog = 5;
    char response[STRING_SIZE];
    char* ok = "ok";
    int bytes;
    char buffer[BUFFER_SIZE];
    struct sockaddr_in server, client;
    socklen_t clientLen = sizeof(client);
    char clientHostname[STRING_SIZE];

    int serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(port);

    bind(serverSocket, (struct sockaddr *)&server, sizeof(server));
    listen(serverSocket, backlog);

    int clientSocket = accept(serverSocket, 
    (struct sockaddr *)&client, &clientLen);

    memset(buffer, 0, BUFFER_SIZE);

    bytes = recv(clientSocket, buffer, BUFFER_SIZE-1, 0);
    if(bytes == -1) {
        perror("recv");
        close(clientSocket);
        close(serverSocket);
        exit(EXIT_FAILURE);
    }

    if(dontAsk) {
       set_verbosity(verbose);
        if(!noNcurses)
            start_windowing();
    
        writeRead(clientSocket);

        close(clientSocket);
        close(serverSocket);
        if(!noNcurses)
            stop_windowing();
        return;
    }
 
    if (getnameinfo((struct sockaddr *)&client, clientLen,
    clientHostname, sizeof(clientHostname), NULL, 0, 0) == 0) {
        printf("Mytalk request from %s@%s. Accept (y/n)?\n", 
        buffer, clientHostname);
    } else {
        printf("Mytalk request from %s@unknown. Accept (y/n)?\n", buffer);
    }
    scanf("%s", response);
    convertToLowercase(response);
    /*change so you convert response to lower*/
    /*response = tolower(response);*/
    if (!strcmp(response, "y") || !strcmp(response, "yes")) {
        bytes = send(clientSocket, ok, 2, 0);
        if(bytes == -1) {
            perror("send");
            close(clientSocket);
            exit(EXIT_FAILURE);
        }
    } else {
        bytes = send(clientSocket, "no", 2, 0);
        if(bytes == -1) {
            perror("send");
            close(clientSocket);
            close(serverSocket);
            exit(EXIT_FAILURE);
        }
        close(clientSocket);
        close(serverSocket);
        exit(EXIT_FAILURE);
    }
    set_verbosity(verbose);
    if(!noNcurses)
        start_windowing();
    
    writeRead(clientSocket);

    close(clientSocket);
    close(serverSocket);
    if(!noNcurses)
        stop_windowing();
}

/*Handles the acitons of the client*/
void clientActions(char* username, char* port) {
    char buffer[BUFFER_SIZE];
    ssize_t bytes;
    int status;
    char* name = "galynch";
    struct addrinfo hints, *result;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    if ((status = getaddrinfo(username, port, &hints, &result)) != 0) {
        fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));
        exit(EXIT_FAILURE);
    }

    int clientSocket = socket(result->ai_family, 
    result->ai_socktype, result->ai_protocol);
    
    printf("Waiting for response from %s\n", username);

    while(connect(clientSocket, result->ai_addr, result->ai_addrlen)) {
        sleep(1);
    }

    memset(buffer, 0, BUFFER_SIZE);
    bytes = send(clientSocket, name, strlen(name), 0);
    if(bytes == -1) {
        perror("send");
        close(clientSocket);
        exit(EXIT_FAILURE);
    }
    bytes = recv(clientSocket, buffer, BUFFER_SIZE-1, 0);
    if(bytes == -1) {
        perror("recv");
        close(clientSocket);
        exit(EXIT_FAILURE);
    }

    if(strcmp(buffer, "ok")) {
        printf("%s declined connection.", username);
        close(clientSocket);
        return;
    }

    set_verbosity(verbose);
    if(!noNcurses)
        start_windowing();
    
    writeRead(clientSocket);

    close(clientSocket);
    if(!noNcurses)
        stop_windowing();
}

/*Handles command line arguments and calling of other functions*/
int main(int argc, char* argv[]) {
    char *endptr;
    int opt;
    int port;
    char portStr[STRING_SIZE];
    char username[STRING_SIZE];
    int base = 10;
    int loc = 1;

    while ((opt = getopt(argc, argv, "vaN")) != -1) {
        switch (opt) {
            case 'v':
                loc++;
                verbose++;
                break;
            case 'a':
                loc++;
                dontAsk = 1;
                break;
            case 'N':
                loc++;
                noNcurses = 1;
                break;
            case '?':
                fprintf(stderr, "Unknown commandline input, %s", 
                "usage mytalk [-v] [-a] [-N] [hostname] port");
                exit(EXIT_FAILURE);
        }
    }
    if(argc-loc == 1) {
        port = strtol(argv[loc], &endptr, base);
        if(endptr == argv[1]) {
            fprintf(stderr, "Error in port number, %s", 
            "usage mytalk [-v] [-a] [-N] [hostname] port");
            exit(EXIT_FAILURE);
        }
        serverActions(port);
    } else if (argc-loc == 2) {
        strcpy(username, argv[loc++]);
        strcpy(portStr, argv[loc]);
        clientActions(username, portStr);
    } else {
        fprintf(stderr, "Error in commandline arguments, %s", 
        "usage mytalk [-v] [-a] [-N] [hostname] port");
        exit(EXIT_FAILURE);
    }
    return 1;
}
