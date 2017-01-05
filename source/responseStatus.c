#include "responseStatus.h"

char *response403 = "HTTP/1.1 403\n"
        "Location: proxy\n"
        "Content-Type: text/html; charset=UTF-8\n"
        "Content-Length: 219\n"
        "\n"
        "<HTML><HEAD><meta http-equiv=\"content-type\" content=\"text/html;charset=utf-8\">\n"
        "<TITLE>403 Forbidden</TITLE></HEAD><BODY>\n"
        "<H1>403 Forbidden</H1>\n"
        "This page was blocked by proxy server, contact to your admin\n"
        "</BODY></HTML>";
