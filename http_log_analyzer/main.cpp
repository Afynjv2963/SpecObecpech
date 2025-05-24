#include <iostream>
#include <fstream>
#include <sstream>
#include <regex>
#include <vector>
#include <unordered_map>
#include <thread>
#include <mutex>
#include <algorithm>
#include <filesystem>
#include <ctime>
#include <nlohmann/json.hpp>

using json = nlohmann::json;
namespace fs = std::filesystem;

// === Структуры ===

struct LogEntry {
    std::string ip;
    std::string datetime;
    std::string method;
    std::string url;
    int code = 0;
    long size = 0;
    std::string referer;
    std::string user_agent;
    time_t timestamp = 0; // для фильтрации по времени
};

struct Config {
    std::string logfile;
    std::string log_format; // common, combined, custom
    std::string regex;
    std::unordered_map<std::string, std::string> fields;
    std::unordered_map<std::string, int> field_indices;

    int topip = 0;
    int topurl = 0;
    int topua = 0;
    bool time_stats = false;
    std::string start_time;
    std::string end_time;
    std::string ip_filter;
    std::string url_filter;
    std::string config_file;

    bool is_valid() const { return !logfile.empty(); }
};

// === Глобальные переменные ===

std::mutex result_mutex;
std::vector<LogEntry> global_logs;

// === Вспомогательные функции ===

std::vector<std::string> split(const std::string& s, char delimiter) {
    std::vector<std::string> tokens;
    std::stringstream ss(s);
    std::string token;
    while (std::getline(ss, token, delimiter)) {
        tokens.push_back(token);
    }
    return tokens;
}

time_t parse_datetime(const std::string& dt_str) {
    struct tm tm = {};
    strptime(dt_str.c_str(), "%d/%b/%Y:%H:%M:%S %z", &tm);
    return mktime(&tm);
}

std::string extract_group(const std::smatch& match, const Config& config, const std::string& field_name) {
    if (config.field_indices.find(field_name) == config.field_indices.end()) {
        throw std::runtime_error("Поле '" + field_name + "' не найдено в конфигурации");
    }

    int idx = config.field_indices.at(field_name);

    if (idx >= match.size() || match[idx].str().empty()) {
        throw std::runtime_error("Группа '" + field_name + "' пустая в строке");
    }

    return match[idx];
}

int safe_stoi(const std::string& str) {
    return str.empty() ? 0 : std::stoi(str);
}

long safe_stol(const std::string& str) {
    return str.empty() ? 0 : std::stol(str);
}

// === Парсинг конфигурации ===

void load_config_from_json(const std::string& filename, Config& config) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        throw std::runtime_error("Не удалось открыть конфигурационный файл: " + filename);
    }

    json j;
    file >> j;

    config.log_format = j.value("log_format", "custom");
    config.regex = j.value("regex", "");
    config.fields.clear();
    config.field_indices.clear();

    if (j.contains("fields")) {
        for (auto& [key, value] : j["fields"].items()) {
            config.fields[key] = value.get<std::string>();
        }
    }

    for (const auto& [field_name, group_index_str] : config.fields) {
        try {
            int idx = std::stoi(group_index_str);
            config.field_indices[field_name] = idx;
        } catch (...) {
            std::cerr << "Ошибка: поле '" << field_name << "' должно быть числом\n";
        }
    }
}

Config parse_cli_args(int argc, char* argv[]) {
    Config config;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--logfile" && i + 1 < argc) {
            config.logfile = argv[++i];
        } else if (arg == "--format" && i + 1 < argc) {
            config.log_format = argv[++i];
        } else if (arg == "--topip" && i + 1 < argc) {
            config.topip = std::stoi(argv[++i]);
        } else if (arg == "--topurl" && i + 1 < argc) {
            config.topurl = std::stoi(argv[++i]);
        } else if (arg == "--topua" && i + 1 < argc) {
            config.topua = std::stoi(argv[++i]);
        } else if (arg == "--time-stats") {
            config.time_stats = true;
        } else if (arg == "--start" && i + 1 < argc) {
            config.start_time = argv[++i];
        } else if (arg == "--end" && i + 1 < argc) {
            config.end_time = argv[++i];
        } else if (arg == "--ip-filter" && i + 1 < argc) {
            config.ip_filter = argv[++i];
        } else if (arg == "--url-filter" && i + 1 < argc) {
            config.url_filter = argv[++i];
        } else if (arg == "--config" && i + 1 < argc) {
            config.config_file = argv[++i];
        }
    }

    return config;
}

// === Регулярные выражения ===

const std::string CLF_REGEX =
    R"delimiter(^(\S+) - - \[([^\]]+)\] "(\w+) (\S+) HTTP/\d\.\d" (\d{3}) (\d+)$)delimiter";

const std::string COMBINED_REGEX =
    R"delimiter(^(\S+) - - \[([^\]]+)\] "(\w+) ([^"]+) HTTP/[\d.]+" (\d{3}) (\d+) "([^"]*)" "([^"]*)"$)delimiter";

// === Парсинг логов ===

void process_chunk(const std::vector<std::string>& lines, const Config& config) {
    std::regex pattern;
    if (config.log_format == "common") {
        pattern = CLF_REGEX;
    } else if (config.log_format == "combined") {
        pattern = COMBINED_REGEX;
    } else {
        pattern = config.regex;
    }

    std::vector<LogEntry> local_logs;

    for (const auto& line : lines) {
        std::smatch match;
        if (!std::regex_match(line, match, pattern)) {
            std::cerr << "Не совпадает с регулярным выражением:\n" << line << "\n";
            continue;
        }

        LogEntry entry;

        try {
            if (config.log_format == "common" || config.log_format == "combined") {
                // Жёстко заданные номера групп для стандартных форматов
                entry.ip = match[1];
                entry.datetime = match[2];
                entry.method = match[3];
                entry.url = match[4];
                entry.code = safe_stoi(match[5]);
                entry.size = safe_stol(match[6]);

                if (config.log_format == "combined" && match.size() > 8) {
                    entry.referer = match[7];
                    entry.user_agent = match[8];
                }
            } else {
                // Для кастомного формата — используем field_indices из JSON
                entry.ip = extract_group(match, config, "ip");
                entry.datetime = extract_group(match, config, "datetime");
                entry.method = extract_group(match, config, "method");
                entry.url = extract_group(match, config, "url");
                entry.code = safe_stoi(extract_group(match, config, "code"));
                entry.size = safe_stol(extract_group(match, config, "size"));

                if (config.field_indices.count("referer"))
                    entry.referer = extract_group(match, config, "referer");

                if (config.field_indices.count("useragent"))
                    entry.user_agent = extract_group(match, config, "useragent");
            }

            entry.timestamp = parse_datetime(entry.datetime);
            local_logs.push_back(entry);

        } catch (const std::runtime_error& e) {
            std::cerr << "Ошибка парсинга строки: " << e.what() << "\n";
            std::cerr << "Строка: " << line << "\n";
        }
    }

    std::lock_guard<std::mutex> lock(result_mutex);
    global_logs.insert(global_logs.end(), local_logs.begin(), local_logs.end());
}

std::vector<LogEntry> parse_log_file_parallel(std::ifstream& file, const Config& config) {
    std::vector<std::thread> threads;
    std::vector<std::vector<std::string>> chunks(4); // 4 потока
    std::string line;
    int chunk_idx = 0;

    while (std::getline(file, line)) {
        chunks[chunk_idx % 4].push_back(line);
        chunk_idx++;
    }

    for (auto& chunk : chunks) {
        if (!chunk.empty()) {
            threads.emplace_back(process_chunk, std::ref(chunk), std::ref(config));
        }
    }

    for (auto& t : threads) {
        t.join();
    }

    return global_logs;
}

// === Аналитика ===

template<typename T>
using Counter = std::unordered_map<T, int>;

template<typename T>
void print_top(const Counter<T>& counts, int N, const std::string& title) {
    std::vector<std::pair<T, int>> sorted(counts.begin(), counts.end());
    std::sort(sorted.begin(), sorted.end(), [](const auto& a, const auto& b) {
        return b.second < a.second;
    });

    std::cout << title << ":\n";
    for (int i = 0; i < N && i < sorted.size(); ++i) {
        std::cout << sorted[i].first << " → " << sorted[i].second << "\n";
    }
    std::cout << "\n";
}

void run_analytics(const std::vector<LogEntry>& logs, const Config& config) {
    Counter<std::string> ip_counter;
    Counter<std::string> url_counter;
    Counter<int> code_counter;
    Counter<std::string> ua_counter;

    for (const auto& entry : logs) {
        ip_counter[entry.ip]++;
        url_counter[entry.url]++;
        code_counter[entry.code]++;
        if (!entry.user_agent.empty())
            ua_counter[entry.user_agent]++;
    }

    if (config.topip > 0)
        print_top(ip_counter, config.topip, "Топ IP");

    if (config.topurl > 0)
        print_top(url_counter, config.topurl, "Топ URL");

    if (config.time_stats) {
        std::map<std::string, int> hour_counter;
        for (const auto& entry : logs) {
            std::istringstream ss(entry.datetime);
            std::string date_part;
            std::getline(ss, date_part, ':');
            std::string hour;
            std::getline(ss, hour, ':');
            hour_counter[hour]++;
        }

        std::cout << "Статистика по часам:\n";
        for (const auto& [h, c] : hour_counter) {
            std::cout << h << ":00 → " << c << "\n";
        }
        std::cout << "\n";
    }

    std::cout << "Распределение кодов ответа:\n";
    for (const auto& [code, count] : code_counter) {
        std::cout << code << " → " << count << "\n";
    }
    std::cout << "\n";

    if (!config.ip_filter.empty() || !config.url_filter.empty()) {
        std::cout << "Фильтрованные записи:\n";
        for (const auto& entry : logs) {
            bool match_ip = config.ip_filter.empty() || entry.ip == config.ip_filter;
            bool match_url = config.url_filter.empty() || entry.url == config.url_filter;
            if (match_ip && match_url) {
                std::cout << entry.ip << " \"" << entry.method << " " << entry.url
                          << "\" " << entry.code << " " << entry.size << "\n";
            }
        }
        std::cout << "\n";
    }
}

// === Основная программа ===

int main(int argc, char* argv[]) {
    try {
        Config config = parse_cli_args(argc, argv);
        if (!config.is_valid()) {
            std::cerr << "Ошибка: неверные аргументы командной строки.\n";
            return 1;
        }

        if (!config.config_file.empty()) {
            load_config_from_json(config.config_file, config);
        }

        std::ifstream log_file(config.logfile);
        if (!log_file.is_open()) {
            throw std::runtime_error("Не удалось открыть файл лога: " + config.logfile);
        }

        std::vector<LogEntry> logs = parse_log_file_parallel(log_file, config);

        run_analytics(logs, config);

    } catch (const std::exception& e) {
        std::cerr << "Ошибка: " << e.what() << "\n";
        return 1;
    }

    return 0;
}