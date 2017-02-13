#ifndef FILTER_PROXY_C_REQUEST_H
#define FILTER_PROXY_C_REQUEST_H

#include <time.h>
#include <stdio.h>
#include <stdlib.h>

struct request{
    struct headerCookie* headers;
    struct headerCookie* cookies;
    int headersCount;
    int cookiesCount;
    char *requestData;
};

struct headerCookie{
    char *name;
    char *value;
    char *cookieAttr;
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

/**
 *
 * @param req request/response structure
 * @param type type od structure (request :  or response : 1 , different cookie header)
 * @return request as string (to send it to server or client)
 */
char *requestToString(struct request req, int type);

#endif //FILTER_PROXY_C_REQUEST_H
