#include "stdio.h"
#include "regex.h"

enum headerRuleType{
    ADD = 0,
    DEL = 1,
    CHA = 2,
};
struct blockRule{
    regex_t hostRegex;
    char* hostPattern;
};
struct headersRule{
    regex_t headerNameRegex;
    char* headerNamePattern;
    char* value;
    regex_t hostNameRegex;
    char* hostNamePattern;
    enum headerRuleType type;
};
struct configStruct{
    struct blockRule* block;
    int blockRulesNumber;
    struct headersRule* header;
    int headerRulesNumber;
    struct headersRule* cookie;
    int cookieRulesNumber;
};


/**
 *
 * @return result from loadConfigWithPath using `../conf.ini` parameter
 */
struct configStruct* loadConfig();
/**
 *
 * @param filePath Path to the config file
 * @return  NULL if config file not exist or is incorrect
 *          pointer to struct configStruct with config definitions
 */
struct configStruct* loadConfigWithPath(char* filePath);
/**
 * Free memory and regex
 * @param config pointer to config
 */
void freeConfig(struct configStruct* config);