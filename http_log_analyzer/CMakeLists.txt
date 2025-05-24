cmake_minimum_required(VERSION 3.10)
project(HttpLogAnalyzer)

set(CMAKE_CXX_STANDARD 17)

add_executable(log_analyzer main.cpp)

# Указание директории заголовков библиотеки nlohmann_json
target_include_directories(log_analyzer PRIVATE /usr/include/nlohmann)

