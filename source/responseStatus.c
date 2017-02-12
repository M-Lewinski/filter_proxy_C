#include "responseStatus.h"

char *response403 = "HTTP/1.1 403\r\n"
        "Location: proxy\r\n"
        "Content-Type: text/html; charset=UTF-8\r\n"
        "Content-Length: 219\r\n"
        "\r\n"
        "<HTML><HEAD><meta http-equiv=\"content-type\" content=\"text/html;charset=utf-8\">\r\n"
        "<TITLE>403 Forbidden</TITLE></HEAD><BODY>\r\n"
        "<H1>403 Forbidden</H1>\r\n"
        "This page was blocked by proxy server, contact to your admin\r\n"
        "</BODY></HTML>";

char *notImplemented = "HTTP/1.1 404\r\n"
        "Location: proxy\r\n"
        "Content-Type: text/html; charset=UTF-8\r\n"
        "Content-Length: 195\r\n"
        "\r\n"
        "<HTML><HEAD><meta http-equiv=\"content-type\" content=\"text/html;charset=utf-8\">\r\n"
        "<TITLE>404 Not implemented</TITLE></HEAD><BODY>\r\n"
        "<H1>404 Not implemented</H1>\r\n"
        "This was not implemented\r\n"
        "</BODY></HTML>";

char* proxyTimeout = "HTTP/1.1 504\r\n"
        "Location: proxy\r\n"
        "Content-Type: text/html; charset=UTF-8\r\n"
        "Content-Length: 189\r\n"
        "\r\n"
        "<HTML><HEAD><meta http-equiv=\"content-type\" content=\"text/html;charset=utf-8\">\r\n"
        "<TITLE>504 Timeout</TITLE></HEAD><BODY>\r\n"
        "<H1>504 Timeout</H1>\r\n"
        "Proxy server timeout server\r\n"
        "</BODY></HTML>";