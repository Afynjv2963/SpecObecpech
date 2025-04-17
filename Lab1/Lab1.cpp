#include <iostream>
#include <regex>


bool isValidEmail(const std::string& email) {
    std::regex email_pattern(R"(^[a-zA-Z0-9._%+-]+@[a-zA-Z0-9.-]+\.[a-zA-Z]{2,}$)");
    return std::regex_match(email, email_pattern);
}

int main(int argc, char* argv[]) {
    std::string email;


    if (argc > 1) {
        email = argv[1];
    }
    else {
        std::cout << "Enter an email address: ";
        std::getline(std::cin, email);
    }

    if (isValidEmail(email)) {
        std::cout << "Valid email address" << std::endl;
    }
    else {
        std::cout << "Invalid email address" << std::endl;
    }

    return 0;
}