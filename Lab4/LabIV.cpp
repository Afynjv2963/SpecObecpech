#include <iostream>
#include <fstream>
#include <regex>
#include <string>
#include <sstream>


std::string readFile(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Ошибка открытия файла: " << filename << std::endl;
        return "";
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

void processAttributes(const std::string& attributes) {
    std::regex attributesReg(R"(([a-z][a-z0-9\-_]*)(?:\s*=\s*(?:"([^"]*)\"|'([^']*)'|([^>\s]*)))?)", std::regex::icase);
    auto attrs_begin = std::sregex_iterator(attributes.begin(), attributes.end(), attributesReg);
    auto attrs_end = std::sregex_iterator();

    for (auto j = attrs_begin; j != attrs_end; ++j) {
        std::smatch attrMatch = *j;
        std::string attrName = attrMatch[1].str();

        if (attrName.empty()) continue;

        std::string attrValue;
        if (attrMatch[2].matched) {
            attrValue = attrMatch[2].str();
        }
        else if (attrMatch[3].matched) {
            attrValue = attrMatch[3].str();
        }
        else if (attrMatch[4].matched) {
            attrValue = attrMatch[4].str();
        }

        if (!attrValue.empty()) {
            std::cout << "  " << attrName << ": " << attrValue << std::endl;
        }
        else {
            std::cout << "  " << attrName << std::endl;
        }
    }
}

void processTags(const std::string& html) {
    std::regex tagsReg("<([a-zA-Z][a-zA-Z0-9]*)([^>]*)>", std::regex::icase);
    auto tags_begin = std::sregex_iterator(html.begin(), html.end(), tagsReg);
    auto tags_end = std::sregex_iterator();

    for (auto i = tags_begin; i != tags_end; ++i) {
        std::smatch tagMatch = *i;
        std::string tagName = tagMatch[1].str();
        std::string attributes = tagMatch[2].str();

        std::cout << "Тэг: <" << tagName << ">" << std::endl;

        if (!attributes.empty()) {
            std::cout << "Аттрибут(ы):" << std::endl;
            processAttributes(attributes);
        }
        else {
            std::cout << "Атрибута(ов) нет" << std::endl;
        }

        std::cout << std::endl;
    }
}

int main(int argc, char* argv[]) {
    setlocale(LC_ALL, "RUSSIAN");
    if (argc < 2) {
        std::cerr << "Использован: " << argv[0] << std::endl;
        return 1;
    }

    std::string html = readFile(argv[1]);
    if (html.empty()) {
        return 1;
    }

    processTags(html);

    return 0;
}
