#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <regex>

using namespace std;

int main() {
    setlocale(LC_ALL, "RUSSIAN");
    cout << "Укажите файл >> " << endl;
    string filepath;
    getline(cin, filepath);

    ifstream file(filepath);
    if (!file.is_open()) {
        cout << "Ошибка! Такого файла нет." << endl;
        return 1;
    }

    string line;
    // +7(XXX)XXX-XX-XX или 8(XXX)XXX-XX-XX
    regex phoneRegex(R"((\+7|8)\(\d{3}\)\d{3}-\d{2}-\d{2}\D)");

    vector<string> phones;

    while (getline(file, line)) {
        auto words_begin = sregex_iterator(line.begin(), line.end(), phoneRegex);
        auto words_end = sregex_iterator();

        for (sregex_iterator i = words_begin; i != words_end; ++i) {
            smatch match = *i;
            phones.push_back(match.str());
        }
    }

    file.close();

    cout << "Номера:" << endl;
    for (const auto& phone : phones) {
        cout << phone.substr(0, phone.length() - 1) << endl;
    }

    return 0;
}
