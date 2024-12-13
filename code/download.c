#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <netdb.h>

#define MAX_BUFFER_SIZE 256

#define SERVER_READY 331
#define SERVER_LOGINSUCCESS 230
#define SERVER_READY_FOR_AUTH 220

typedef struct {
    char user[MAX_BUFFER_SIZE];
    char password[MAX_BUFFER_SIZE];
    char host[MAX_BUFFER_SIZE];
    char resource[MAX_BUFFER_SIZE];
    char file[MAX_BUFFER_SIZE];
    char ip[MAX_BUFFER_SIZE];
} URL;

typedef enum {
    START,
    SINGLE,
    MULTIPLE,
    END
} ResponseState;

int parse_url(const char *input, URL *url) {
    if (strlen(input) == 0) {
        printf("Invalid URL: The URL is empty\n");
        return -1;
    }

    // Initialize URL with default values
    strcpy(url->user, "anonymous");
    strcpy(url->password, "password");

    // ftp://[<user>:<password>@]<host>/<url-path>
    // Get the host name (that starts after "://")
    const char *host_start = strstr(input, "://");
    if (host_start) {
        host_start += 3; // Skip "://"
    }
    else {
        host_start = input; // No "://" found
    }

    // Get the user and password (if any)
    const char *at_sign = strchr(host_start, '@');
    if (at_sign) {
        sscanf(host_start, "%255[^:]:%255[^@]@", url->user, url->password); 
        host_start = at_sign + 1; 
    }

    // Get the host and resource
    const char *slash = strchr(host_start, '/');
    if (slash) {
        strncpy(url->host, host_start, slash - host_start);
        url->host[slash - host_start] = '\0';
        strcpy(url->resource, slash + 1);
    }
    // If there is no resource, the host is the last part
    else {
        strcpy(url->host, host_start);
        strcpy(url->resource, "");
    }

    // Get the file name (the last part of the resource)
    char *file_start = strrchr(url->resource, '/');
    if (file_start) {
        strcpy(url->file, file_start + 1);
    } else {
        strcpy(url->file, url->resource);
    }

    // Get the IP address from the host name
    struct hostent *h;
    if (strlen(url->host) == 0) {
        // The host name is empty
        printf("Invalid host name: The host name is empty\n");
        return -1;
    }
    if ((h = gethostbyname(url->host)) == NULL) {
        printf("Invalid hostname '%s'\n", url->host);
        return -1;
    }

    // Save the IP address
    strcpy(url->ip, inet_ntoa(*((struct in_addr *) h->h_addr)));

    return 0;
}

int create_socket(const char *ip, int port) {
    int sockfd;
    struct sockaddr_in server_addr;

    bzero((char *) &server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(ip);
    server_addr.sin_port = htons(port);

    // Open a TCP socket
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket()");
        return -1;
    }

    // Connect to the server
    if (connect(sockfd, (struct sockaddr *) &server_addr, sizeof(server_addr)) < 0) {
        perror("connect()");
        return -1;
    }

    return sockfd;
}

int read_response(const int socket, char* buffer) {
    char byte;
    int index = 0, responseCode;
    ResponseState state = START;
    memset(buffer, 0, MAX_BUFFER_SIZE);

    while (state != END) {
        
        read(socket, &byte, 1);
        switch (state) {
            case START:
                if (byte == ' ') state = SINGLE;
                else if (byte == '-') state = MULTIPLE;
                else if (byte == '\n') state = END;
                else buffer[index++] = byte;
                break;
            case SINGLE:
                if (byte == '\n') state = END;
                else buffer[index++] = byte;
                break;
            case MULTIPLE:
                if (byte == '\n') {
                    memset(buffer, 0, MAX_BUFFER_SIZE);
                    state = START;
                    index = 0;
                }
                else buffer[index++] = byte;
                break;
            case END:
                break;
            default:
                break;
        }
    }

    sscanf(buffer, "%d", &responseCode);
    return responseCode;
}

int authenticate(int sockfd, const char *user, const char *password) {

    char userCommand[5+strlen(user)+1]; sprintf(userCommand, "user %s\n", user);
    char passCommand[5+strlen(password)+1]; sprintf(passCommand, "pass %s\n", password);
    char answer[MAX_BUFFER_SIZE];
    
    write(sockfd, userCommand, strlen(userCommand));
    int response = read_response(sockfd, answer);

    // Handle the initial server response
    if (response == SERVER_READY_FOR_AUTH) {
        // Server is ready, send the user command again
        write(sockfd, userCommand, strlen(userCommand));
        response = read_response(sockfd, answer);
    }

    if (response == SERVER_READY) {
        // Send the password command
        write(sockfd, passCommand, strlen(passCommand));
        response = read_response(sockfd, answer);
    }

    if (response != SERVER_READY) {
        printf("ERROR authenticate: %s\n", answer);
        exit(-1);
    }

    write(sockfd, passCommand, strlen(passCommand));
    return read_response(sockfd, answer);
}


int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Usage: %s <url>\n", argv[0]);
        return 1;
    }

    URL url;
    if (parse_url(argv[1], &url) < 0) {
        return 1;
    }

    /*
    printf("User: %s\n", url.user);
    printf("Password: %s\n", url.password);
    printf("Host: %s\n", url.host);
    printf("Resource: %s\n", url.resource);
    printf("File: %s\n", url.file);
    printf("IP: %s\n", url.ip);
    */

    // Server address handling
    int sockfd;
    struct sockaddr_in server_addr;
    char buf[] = "Mensagem de teste na travessia da pilha TCP/IP\n";
    size_t bytes;

    bzero((char *) &server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(url.ip);
    server_addr.sin_port = htons(21); 

    // Open a TCP socket
    int socketA;
    if ((socketA = create_socket(url.ip, 21)) < 0) {
        printf("Error creating socket: Socket to %s and port %d failed\n", url.ip, 21);
        return 1;
    }

    // Authenticate
    if (authenticate(socketA, url.user, url.password) != SERVER_LOGINSUCCESS) {
        printf("Error authenticating: Username %s and password %s\n", url.user, url.password);
        return 1;
    }

    return 0;
}