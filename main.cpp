#include <iostream>
#include "utility.h"

using namespace std;
using namespace tinyxml2;

int main(int argc, char **argv) {
    HTTPUtility util;
    string url ("http://help.websiteos.com/websiteos/example_of_a_simple_html_page.htm");
    string html = util.sendRequest(url);
    string xml = util.convertHTML(html.c_str());
    XMLDocument dom;
    dom.Parse(xml.c_str());
    util.extractContent(&dom, 0);
}
