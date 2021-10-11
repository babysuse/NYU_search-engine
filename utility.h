#ifndef HTTP_UTILITY_H
#define HTTP_UTILITY_H
#include "tinyxml2.h"

#include <tidy/tidyenum.h>
#include <string>

class HTTPUtility {
    public:
        static std::string sendRequest(const std::string);
        static std::string convertHTML(const char *, TidyOptionId tidyOptId = TidyXmlOut);
        static void extractContent(tinyxml2::XMLNode *, int);
        static size_t getMemUsage();
    private:
        static size_t writeBuffer(char *data, size_t size, size_t nmemb, std::string *buffer);
};

#endif
