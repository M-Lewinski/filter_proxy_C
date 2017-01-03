#include <stdio.h>
#include <stdlib.h>
#include <regex.h>
#include <string.h>

enum ruleType{
    ADD = 0,
    DEL = 1,
    CHA = 2,
};
struct blockRule{
    regex_t* hostRegex;
    char* hostPattern;
};
struct headersRule{
    regex_t* nameRegex;
    char* namePattern;
    char* value;
    regex_t* hostNameRegex; //OPTIONAL
    char* hostNamePattern;
    enum ruleType type;
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
/**
 * Parse single line from config file
 * @param config pointer to config struct
 * @param line buffer with line
 * @param lineLen line length
 * @return 0 if everything was OK, sth else in other case
 */
int parseLine(struct configStruct* config, char* line, ssize_t lineLen);

/**
 * Parse block rule. Used in parseLine function;
 * @param config pointer to configStruct to save results
 * @param splittedLine splittedLine
 * @return 0 if everything was OK, sth else in other case
 */
int parseBlockRule(struct configStruct* config, char** splittedLine);
/**
 * Parse header or cookie rule. Used in parseLine function;
 * @param config pointer to configStruct to save results
 * @param splittedLine
 * @param headerCookie
 * @return 0 if everything was ok, sth else in other case
 */
int parseHeaderRule(struct configStruct* config, char** splittedLine, int headerCookie);
