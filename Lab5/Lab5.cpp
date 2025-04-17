#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <iomanip>
#include <algorithm>

std::vector<std::string> parseCSVLine(const std::string& line) {
    std::vector<std::string> fields;
    std::string field;
    bool inQuotes = false;

    for (size_t i = 0; i < line.size(); ++i) {
        char c = line[i];

        if (c == '"') {
            if (inQuotes && i + 1 < line.size() && line[i + 1] == '"') {
                field += '"';
                ++i;
            }
            else {
                inQuotes = !inQuotes;
            }
        }
        else if (c == ',' && !inQuotes) {
            fields.push_back(field);
            field.clear();
        }
        else {
            field += c;
        }
    }
    fields.push_back(field);

    return fields;
}

int main(int argc, char* argv[]) {
    setlocale(LC_ALL, "RUSSIAN");
    if (argc < 2) {
        std::cerr << "Использование: " << argv[0] << " <имя_CSV_файла>" << std::endl;
        return 1;
    }

    std::ifstream file(argv[1]);
    if (!file.is_open()) {
        std::cerr << "Ошибка открытия файла: " << argv[1] << std::endl;
        return 1;
    }

    std::vector<std::vector<std::string>> table;
    std::string line;
    while (std::getline(file, line)) {
        table.push_back(parseCSVLine(line));
    }

    file.close();

    if (table.empty()) {
        std::cerr << "Файл пуст или имеет неправильный формат" << std::endl;
        return 1;
    }

    size_t numColumns = table[0].size();
    std::vector<size_t> columnWidths(numColumns, 0);
    for (const auto& row : table) {
        for (size_t i = 0; i < row.size(); ++i) {
            columnWidths[i] = std::max(columnWidths[i], row[i].length());
        }
    }

    for (size_t rowIdx = 0; rowIdx < table.size(); ++rowIdx) {
        const auto& row = table[rowIdx];

        for (size_t i = 0; i < row.size(); ++i) {
            std::cout << std::setw(columnWidths[i]) << std::left << row[i];
            if (i < row.size() - 1) std::cout << " : ";
        }
        std::cout << std::endl;

        if (rowIdx == 0) {
            for (size_t i = 0; i < numColumns; ++i) {
                std::cout << std::string(columnWidths[i], '-');
                if (i < numColumns - 1) std::cout << "-+-";
            }
            std::cout << std::endl;
        }
    }

    return 0;
}
