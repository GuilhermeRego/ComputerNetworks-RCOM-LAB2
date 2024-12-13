/**
 * Example code for getting the IP address from hostname.
 * tidy up includes
 */

// getip.c: receives as a parameter a host name and returns its IP address


#include "getip.h"

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
