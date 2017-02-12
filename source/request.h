#ifndef FILTER_PROXY_C_REQUEST_H
#define FILTER_PROXY_C_REQUEST_H

#include <time.h>
#include <stdio.h>
#include <stdlib.h>

struct request{
    struct header* headers;
    int headersCount;
    char *requestData;
};
struct header{
    char name[60];
    char *value;
};

struct requestStruct{
    int clientSoc;
    int serverSoc;
    struct request* clientRequest;
    struct request* serverResponse;
    time_t time;
};

/**
 * Create new request structure and allocate memory
 * @return pointer to request structure
 */
struct requestStruct * newRequestStruct();
/**
 * Create new request and allocate memory
 * @return pointer to request
 */
struct request * newRequest();

#endif //FILTER_PROXY_C_REQUEST_H
