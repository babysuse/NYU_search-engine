#include "utility.h"

#include <cstdlib>
#include <iostream>
#include <fstream>

using namespace std;
using namespace tinyxml2;

string HTTPUtility::sendRequest(const string url) {
    CURL *curl = curl_easy_init();
    string header;
    string response;
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "curl/7.64.1");
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &HTTPUtility::writeBuffer);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(curl, CURLOPT_HEADERDATA, &header);
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());

    CURLcode result = curl_easy_perform(curl);
    if (result != CURLE_OK) {
        std::cerr << "curl_easy_perform() failed:" << curl_easy_strerror(result) << std::endl;
        exit(1);
    }
    return response;
}

string HTTPUtility::convertHTML(const char *html, TidyOptionId tidyOptId) {
    TidyDoc tdoc = tidyCreate();
    TidyBuffer tidyBuffer = {0};
    TidyBuffer tidyErrbuf = {0};
    tidyOptSetBool(tdoc, TidyQuiet, yes);
    tidyOptSetBool(tdoc, TidyNumEntities, yes);
    tidyOptSetBool(tdoc, TidyShowWarnings, no);
    tidyOptSetBool(tdoc, tidyOptId, yes);
    tidySetErrorBuffer(tdoc, &tidyErrbuf);

    tidyParseString(tdoc, html);
    tidyCleanAndRepair(tdoc);
    tidySaveBuffer(tdoc, &tidyBuffer);

    string result ((char *) tidyBuffer.bp);
    tidyBufFree(&tidyBuffer);
    tidyRelease(tdoc);
    return result;
}

void HTTPUtility::extractContent(XMLNode* node, int layer) {
    for (XMLElement *child = node->FirstChildElement(); child; child = child->NextSiblingElement()) {
        string indent (layer, '\t');
        const char *text = child->GetText();
        
        cout << indent << child->Value() << " | ";
        // append indent to all \n
        if (text) {
            size_t pos = 0;
            string content (text);
            string str = indent + "\t";
            if (content[0] == '\n')
                content.erase(0, 1);
            while ((pos = content.find("\n", pos)) != string::npos) {
                content.insert(pos + 1, str);
                pos += str.size();
            }
            cout << endl << str << "\"" << content << "\"" << endl;
        } else {
            cout << "-" << endl;
        }
        extractContent(child, layer + 1);
    }
}

size_t HTTPUtility::writeBuffer(char *data, size_t size, size_t nmemb, std::string *buffer) {
    buffer->append((char *) data, size * nmemb);
    return size * nmemb;
}
