#include "request.h"
#include <regex.h>
#include "string.h"
#include "config.h"

#ifndef FILTER_PROXY_C_RULE_H
#define FILTER_PROXY_C_RULE_H

/**
 *
 * @param toCheck string to check
 * @param regex pointer to regex pattern
 * @return 1 if match 0 if not match
 */
int checkRegex(char* toCheck, regex_t *regex);

/**
 *
 * @param config pointer co config struct
 * @param reqStruct structore with request (headers, data)
 * @return 1 if blocked, 0 if not blocked
 */
int checkBlocked(struct configStruct *config, struct requestStruct *reqStruct);

/**
 *
 * @param config pointer co config struct
 * @param reqStruct pointer to request struct (request in this struct will be modified)
 */
void filterRequest(struct configStruct *config, struct requestStruct *reqStruct);

/**
 *
 * @param config pointer co config struct
 * @param reqStruct pointer to request struct (response in this struct will be modified)
 */
void filterResponse(struct configStruct *config, struct requestStruct *reqStruct);



#endif //FILTER_PROXY_C_RULE_H
