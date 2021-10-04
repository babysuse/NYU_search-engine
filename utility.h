#ifndef HTTP_UTILITY_H
#define HTTP_UTILITY_H
#include "tinyxml2.h"

#include <tidy/tidy.h>
#include <tidy/buffio.h>
#include <tidy/tidyenum.h>
#include <curl/curl.h>
#include <string>

class HTTPUtility {
    public:
        std::string sendRequest(const std::string);
        std::string convertHTML(const char *, TidyOptionId tidyOptId = TidyXmlOut);
        void extractContent(tinyxml2::XMLNode *, int);
    private:
        static size_t writeBuffer(char *data, size_t size, size_t nmemb, std::string *buffer);
};

#endif
