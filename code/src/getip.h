#ifndef GETIP_H
#define GETIP_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <arpa/inet.h>

#define MAX_BUFFER_SIZE 256

typedef struct {
    char user[MAX_BUFFER_SIZE];
    char password[MAX_BUFFER_SIZE];
    char host[MAX_BUFFER_SIZE];
    char resource[MAX_BUFFER_SIZE];
    char file[MAX_BUFFER_SIZE];
    char ip[MAX_BUFFER_SIZE];
} URL;

int parse_url(const char *input, URL *url);

#endif // GETIP_H