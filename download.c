#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <netdb.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>

#define FTP_PORT 21

#define MAX_BUFFER_SIZE 255
#define MAX_COMMAND_LENGTH 512

#define AT "@"
#define SLASH "/"

int sockfdB;

typedef struct {
    char user[MAX_BUFFER_SIZE];
    char password[MAX_BUFFER_SIZE];
    char host[MAX_BUFFER_SIZE];
    char path[MAX_BUFFER_SIZE];
    char filename[MAX_BUFFER_SIZE];
    char ip[MAX_BUFFER_SIZE];
} URL;

// Get the ip address of a host	
int getip(char host[], URL* url) {
    // Check if the url is NULL
    if (url == NULL) {
        printf("ERROR: url is NULL\n");
        return 1;
    }

    // Get the host by name
	struct hostent *h;
	if ((h = gethostbyname(host)) == NULL) {
		printf("ERROR: gethostbyname() failed, h is NULL\n");
		return 1;
	}

	// Copy the ip address to the url struct
    strcpy(url->ip, inet_ntoa(*((struct in_addr *)h->h_addr_list[0])));
	return 0;
}

// Create a struct url with all fields set to 0
int create_url_struct(URL* url) {
    // Check if the url is NULL
    if (url == NULL) {
        printf("ERROR: url is NULL\n");
        return 1;
    }

    // Set all fields to 0
    memset(url->user, 0, MAX_BUFFER_SIZE);
    memset(url->password, 0, MAX_BUFFER_SIZE);
    memset(url->host, 0, MAX_BUFFER_SIZE);
    memset(url->path, 0, MAX_BUFFER_SIZE);
    memset(url->filename, 0, MAX_BUFFER_SIZE);
    memset(url->ip, 0, MAX_BUFFER_SIZE);
    return 0;
}

// Parse the filename from the url path
int parse_URL(char *input, URL *url) {
    // Check if the input or url is NULL
    if (input == NULL || url == NULL) {
        printf("ERROR: input or url is NULL\n");
        return 1;
    }

    // Variables initialization
	char *user, *password, *host, *path;
    int length = strlen(input);

	// Input needs to be in the following format: ftp://[<user>:<password>@]<host>/<url-path>
    if (input[0] != 'f' || input[1] != 't' || input[2] != 'p' || input[3] != ':' || input[4] != '/' || input[5] != '/') {
        printf("ERROR: parse_URL, URL Doesn't start with 'ftp://'\n");
        return 1;
    }

    // Check if the url has the user and password (before the @)
	int hasUserAndPassword = 0;
    for (int i = 0; i < length; i++) {
        if (input[i] == '@') {
            hasUserAndPassword = 1;
            break;
        }
    }

    // If the url has the user and password we use them
    if (hasUserAndPassword) {
        //Get user
        char user_delimiter[] = ":";
        user = strtok(&input[6], user_delimiter);
        strcpy(url->user, user);

        //Get password
        char pass_delimiter[] = AT;
        int password_index = strlen(user) + 7;
        password = strtok(&input[password_index], pass_delimiter);
        strcpy(url->password, password);

        //Get host
        char host_delimiter[] = SLASH;
        int host_index = password_index + strlen(password) + 1;
        host = strtok(&input[host_index], host_delimiter);
        strcpy(url->host, host);

        //Get path
        char url_delimiter[] = "\n";
        int url_index = host_index + strlen(host) + 1;
        path = strtok(&input[url_index], url_delimiter);
        strcpy(url->path, path);
    }

    // If the url does not have the user and password we use the default ones
    else {
        // Set the default user (anonymous)
		memset(url->user, 0 ,sizeof(url->user));
		strcpy(url->user, "anonymous");

        // Set the default password (<empty>)
		memset(url->password, 0 ,sizeof(url->password));
		strcpy(url->password, "");

        //Get host
        const char host_delimiter[] = "/";
        host = strtok(&input[6], host_delimiter);
        strcpy(url->host, host);

        //Get url-path
        const char url_delimiter[] = "\n";
        int url_index = strlen(host) + 7;
        path = strtok(&input[url_index], url_delimiter);
        strcpy(url->path, path);
    }
    return 0;
}

// Parse the filename from the path (last part of the path)
int parseFilename(char *path, URL *url) {
    // Check if the path or url is NULL
    if (path == NULL || url == NULL) {
        printf("ERROR: path or url is NULL\n");
        return 1;
    }

    // Parse the filename from the path
    char filename[MAX_BUFFER_SIZE];
    char path_to_remove[MAX_BUFFER_SIZE];
    strcpy(filename, path);

    // While there is a slash in the filename
    while (strchr(filename, '/')) {
        const char path_deli[] = SLASH;
        strcpy(path_to_remove, strtok(&filename[0], path_deli));
        strcpy(filename, filename + strlen(path_to_remove) + 1);
    }
    strcpy(url->filename, filename);

     if (url->filename == NULL || strlen(url->filename) == 0) {
         printf("ERROR: url->filename is NULL\n");
         return 1;
    }

    return 0;
}

// Create a socket and connect it to the server
int create_socket(char* ip, int port) {
    // Check if the ip address is NULL
    if (ip == NULL) {
        printf("ERROR: ip is NULL\n");
        return -1;
    }

    // Create the socket
    int sockfd;
    struct sockaddr_in server_addr;
    bzero((char*) &server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(ip);
    server_addr.sin_port = htons(port);

    // Open the TCP Socket
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("ERROR: Failed to Open TCP Socket\n");
        return -1;
    }

    // Connect the socket to the server
    if (connect(sockfd, (struct sockaddr *) &server_addr, sizeof(server_addr)) < 0) {
        perror("ERROR: Failed to connect socket to server\n");
        return -1;
    }
    return sockfd;
}

// Read the response from the server
int read_socket(int sockfd, char* response) {
    if (response == NULL) {
        printf("ERROR: response is NULL\n");
        return 1;
    }

    // Read the response from the server
    FILE* fp = fdopen(sockfd, "r");
    memset(response, 0, MAX_BUFFER_SIZE);
    response = fgets(response, MAX_BUFFER_SIZE, fp);

    // Check if the response is valid (starts with a number between 1 and 5 and has a space in the 4th position)
    while (!('1' <= response[0] && response[0] <= '5') || response[3] != ' ') {
        memset(response, 0, MAX_BUFFER_SIZE);
        response = fgets(response, MAX_BUFFER_SIZE, fp);
    }
    // response[0] >= 1 && response[0] <= 5 means received a status line
    // response[3] == ' ' means received a last status line
    // response [0] <= 5 means received numerated status line
    return 0;
}

// Send a command to the server writing it to the socket
int ftp_send_command(int sockfd, const char* command, int command_size, char* response) {
    // Check if the command or response is NULL
    if (command == NULL || response == NULL) {
        printf("ERROR: command or response is NULL\n");
        return 1;
    }

    // Write the command to the socket
    int res = write(sockfd, command, command_size);
    if (res <= 0) {
        printf("ERROR: write()\n");
        return 1;
    }

    // Read the response from the server
    if (read_socket(sockfd, response) != 0) {
        printf("ERROR: read_socket()\n");
        return 1;
    }
    return 0;
}

// Enter passive mode
int ftp_passive_mode(int sockfdA, char* command, size_t command_size, char* response) {
    // Check if the command or response is NULL
    if (command == NULL || response == NULL) {
        printf("ERROR: command or response is NULL\n");
        return 1;
    }

    // Send the command to the server
    if (ftp_send_command(sockfdA, command, command_size, response)) {
        printf("ERROR: ftp_send_command()");
        return 1;
    }

    // Variables initialization
    char ip[MAX_BUFFER_SIZE];
    int port_num;
    int ip1, ip2, ip3, ip4;
    int port1, port2;

    // Get the ip address and port number
    if (sscanf(response, "227 Entering Passive Mode (%d,%d,%d,%d,%d,%d)", &ip1, &ip2, &ip3, &ip4, &port1, &port2) < 0) {
        printf("ERROR: Failed getting IP Address and Port number for Passive Mode\n");
        return 1;
    }

    // Creating server ip address
    sprintf(ip, "%d.%d.%d.%d", ip1, ip2, ip3, ip4);

    // Calculating tcp port number
    port_num = port1 * 256 + port2;

    // Create the passive mode socket
    if ((sockfdB = create_socket(ip, port_num)) < 0) {
        printf("ERROR: Failed to Open Passive Mode Socket\n");
        return 1;
    }
    return sockfdB;
}

// Send the retrieve command to the server
int ftp_retr(int socket_id, const char* command, int command_size) {
    // Check if the command is NULL
    if (command == NULL) {
        printf("ERROR: command is NULL\n");
        return 1;
    }

    // Write the command to the socket
    int res = write(socket_id, command, command_size);
    if (res <= 0) {
        printf("ERROR: ftp_retr()\n");
        return 1;
    }

    return 0;
}

// Retrieve the file from the server
int ftp_retrieve_file(int socket_id, char* filepath, char* command, char* response) {
    // Check if the filepath, command or response is NULL
    if (filepath == NULL || command == NULL || response == NULL) {
        printf("ERROR: filepath, command or response is NULL\n");
        return 1;
    }

    // Send the retrieve command to the server
    sprintf(command, "RETR %s\r\n", filepath);
    if (ftp_retr(socket_id, command, strlen(command))){
        printf("ERROR: ftp_retr()\n");
        return 1;
    }

    return 0;
}

// Download the file from the server
int ftp_download_file(int sockfdB, char* filename) {
    // Check if the filename is NULL
    if (filename == NULL || strlen(filename) == 0) {
        printf("ERROR: filename is NULL\n");
        return 1;
    }

    // Variables initialization
    char buffer[MAX_BUFFER_SIZE];
    int file_fd;
    int res;

    // Open the file
    if((file_fd = open(filename, O_WRONLY | O_CREAT, 0666)) < 0) {
        perror("open()\n");
        return 1;
    }

    // Read the file from the server
    while ((res = read(sockfdB, buffer, sizeof(buffer)) > 0)) {
        // Check for any errors on the read
        if (res < 0) {
            printf("ERROR: read()\n");
            return 1;
        }

        // Write the file to the buffer
        if (write(file_fd, buffer, res) < 0) {
            perror("ERROR: write()\n");
            return 1;
        }

    }

    // Close the file and the socket
    close(file_fd);
    close(sockfdB);
    return 0;
}

// Authenticate the user with the server
int authenticate(const URL url, char* command, char* response, int sockfdA) {
    // Check if the url, command or response is NULL
    if (url.user == NULL || url.password == NULL || command == NULL || response == NULL) {
        printf("ERROR: url, command or response is NULL\n");
        return 1;
    }

    // Get user command
    sprintf(command, "USER %s\r\n", url.user);
    // Send user command
    if (ftp_send_command(sockfdA, command, strlen(command), response) != 0){
        printf("ERROR: Failed to send USER command\n");
        return 1;
    }

    // Get password command
    bzero(command, MAX_COMMAND_LENGTH);
    bzero(response, MAX_BUFFER_SIZE);
    sprintf(command, "PASS %s\r\n", url.password);

    // Send password command
    if(ftp_send_command(sockfdA, command, strlen(command), response)){
        printf("ERROR: Failed to send PASS command\n");
        return 1;
    }

    return 0;
}

int main(int argc, char** argv){
	// Check the user input
	if(argc != 2){
		printf("Usage: %s <url>\n", argv[0]);
		return 1;
	}

    // Variables initialization
	char command[MAX_COMMAND_LENGTH];
	char response[MAX_BUFFER_SIZE];
    URL url;

    // Create the url struct
    if (create_url_struct(&url) != 0) {
		printf("ERROR: Failed to create URL Struct\n");
		return 1;
	}

    // Parse URL
    if (parse_URL(argv[1], &url) != 0) {
        printf("ERROR: Failed to Parse URL\n");
        return 1;
    }

    // Get the ip address from the host
    char *path = url.path;
    if (getip(url.host, &url) != 0) {
        printf("ERROR: getip()\n");
        return 1;
    }
	if (parseFilename(path, &url)) {
        printf("ERROR: Failed to Parse Filename\n");
        return 1;
    }

    printf("Username: %s\n", url.user);
	printf("Password: %s\n", url.password);
	printf("Host: %s\n", url.host);
	printf("Path: %s\n", url.path);
    printf("Filename: %s\n", url.filename);
    printf("IP: %s\n\n", url.ip);

    // 1: Create the socketA
	int sockfdA;
	if((sockfdA = create_socket(url.ip, FTP_PORT)) < 0){
		printf("ERROR: create_socket()\n");
        return 1;
	}

    // Read the response from the server
	if (read_socket(sockfdA, response) != 0) {
        printf("ERROR: read_socket()\n");
        return 1;
    }
    printf("Socket created\n");

	// 2: Authenticate the user
    if (authenticate(url, command, response, sockfdA) != 0) {
        printf("ERROR: authenticate()\n");
        return 1;
    }
    printf("User authenticated\n");

	// 3: Get passive mode command
	bzero(command, MAX_COMMAND_LENGTH);
	bzero(response, MAX_BUFFER_SIZE);
	sprintf(command, "PASV\r\n");
    // Send passive mode command
	if (ftp_passive_mode(sockfdA, command, strlen(command), response) < 0) {
		printf("ERROR: Failed to enter passive mode\n");
        return 1;
    }
    printf("Passive mode activated\n");

	// 4: Retrieve file
	char filepath[MAX_BUFFER_SIZE];
	bzero(command, MAX_COMMAND_LENGTH);
	bzero(response, MAX_BUFFER_SIZE);
    sprintf(filepath, "%s", url.path);
	if (ftp_retrieve_file(sockfdA, filepath, command, response)) {
        printf("ERROR: ftp_retrieve_file()\n");
        return 1;
    }
    printf("Downloading...\n");

	// 5: Download file
	if (ftp_download_file(sockfdB, url.filename) != 0) {
        printf("ERROR: ftp_download_file()\n");
        return 1;
    }
    printf("Download of file %s complete!\n", url.filename);

	close(sockfdA);
    return 0;
}

