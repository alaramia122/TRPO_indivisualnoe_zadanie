#include "Header.h"

bool contains_only(std::string const& str, char flag) {
    switch (flag) {
    case 'б':
        // Проверяем кириллицу и латиницу.
        return str.find_first_not_of(
            "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ"
            "абвгдеёжзийклмнопрстуфхцчшщъыьэюя"
            "АБВГДЕЁЖЗИЙКЛМНОПРСТУФХЦЧШЩЪЫЬЭЮЯ") == std::string::npos;

    case 'р':
        // Проверяем только кириллицу.
        return str.find_first_not_of(
            "абвгдеёжзийклмнопрстуфхцчшщъыьэюя"
            "АБВГДЕЁЖЗИЙКЛМНОПРСТУФХЦЧШЩЪЫЬЭЮЯ") == std::string::npos;

    case 'а':
        // Проверяем только латиницу.
        return str.find_first_not_of(
            "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ") ==
            std::string::npos;

    case 'ц':
        // Проверяем, что строка содержит только цифры.
        return str.find_first_not_of("0123456789") == std::string::npos;

    case 'з':
        // Исключаем знаки конца предложения.
        return str.find_first_of(R"(.!?"'… )") == std::string::npos;

    case 'с':
        // Исключаем специальные символы и знаки пунктуации.
        return str.find_first_of(R"(,,></|=+&^%$#@*(){}[]№;:'»«\)") ==
            std::string::npos;

    default:
        return false;
    }
}

std::vector<std::string> Cutter_of_string(
    const std::string& stroka,
    char razdel) {
    std::stringstream stream(stroka);
    std::string part;
    std::vector<std::string> result;

    while (getline(stream, part, razdel)) {
        result.push_back(part);
    }

    return result;
}

std::vector<char> Cutter_of_string_char(
    const std::string& stroka,
    char razdel) {
    std::stringstream stream(stroka);
    std::string part;
    std::vector<char> result;

    while (getline(stream, part, razdel)) {
        if (!part.empty()) {
            result.push_back(part.front());
        }
    }

    return result;
}

Input_data input_data_reader(bool flag, bool flag2, bool flag3) {
    Input_data data;
    char ch;

    do {
        ch = _getch();

        switch (ch) {
        case 27:
            data.Esc_controller = true;
            flag = false;
            break;

        case 13:
            data.Enter_controller = true;
            flag = false;
            break;

        case 8:
            if (!data.result.empty()) {
                data.result.pop_back();
                std::cout << "\b \b";
            }
            break;

        case -32:
            if (flag3) {
                ch = _getch();
            } else {
                data.result += ch;

                if (flag2) {
                    std::cout << ch;
                }
            }
            break;

        default:
            data.result += ch;

            if (flag2) {
                std::cout << ch;
            }
            break;
        }
    } while (flag);

    return data;
}
