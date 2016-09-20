#include <fstream>
#include <string>
#include <cstring>
#include <vector>
#include "mdtransform.h"
using namespace std;

int main()
{
    MarkdownTransform transformer("test.md");

    std::string table = transformer.getTableOfContents();

    std::string contents = transformer.getContents();

    std::string head = "<!DOCTYPE html>\n<html>\n<head>\
        \n<meta charset=\"utf-8\">\
        \n<title>Markdown</title>\
        \n<link rel=\"stylesheet\" href=\"github-markdown.css\">\
        \n</head>\n<body>\n<article class=\"markdown-body\">";
    std::string end = "\n</article>\n</body>\n</html>";

    std::ofstream out;
    out.open("output/index.html");
    out << head + table + contents + end;
    out.close();

    return 0;
}
