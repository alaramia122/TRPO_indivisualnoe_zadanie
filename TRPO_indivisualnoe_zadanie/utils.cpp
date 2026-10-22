#include "Header.h"

bool contains_only(std::string const& str, char flag) {
    switch (flag)
    {
        //только буквы (кириллица + латиница)
    case 'б':	return str.find_first_not_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZабвгдеёжзийклмнопрстуфхцчшщъыьэюяАБВГДЕЁЖЗИЙКЛМНОПРСТУФХЦЧШЩЪЫЬЭЮЯ") == std::string::npos;
        break;
        //только кириллица
    case 'р':	return str.find_first_not_of("абвгдеёжзийклмнопрстуфхцчшщъыьэюяАБВГДЕЁЖЗИЙКЛМНОПРСТУФХЦЧШЩЪЫЬЭЮЯ") == std::string::npos;
        break;
        //только латиница
    case 'а':	return str.find_first_not_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ") == std::string::npos;
        break;
        //только цифры
    case 'ц':	return str.find_first_not_of("0123456789") == std::string::npos;
        break;
        //все кроме знаков препинания-концов предложения
    case 'з':	return str.find_first_of(".!?\"'…") == std::string::npos;
        break;
        //все кроме спецсимволов и знаков пунктуации
    case 'с':	return str.find_first_of(",,></|=+&^%$#@*(){}[]№;:'»«\\") == std::string::npos;
        break;
    default:	return false;
        break;
    }
}

std::vector<std::string> Cutter_of_string(const std::string& stroka, char razdel) {
    std::stringstream ss(stroka);
    std::string s;
    std::vector<std::string> out;
    while (getline(ss, s, razdel)) out.push_back(s);
    return out;
}

std::vector<char> Cutter_of_string_char(const std::string& stroka, char razdel) {
    std::stringstream ss(stroka);
    std::string s;
    std::vector<char> out;
    while (getline(ss, s, razdel)) if (!s.empty()) out.push_back(*s.c_str());
    return out;
}

Input_data input_data_reader(bool flag, bool flag2, bool flag3) {
    Input_data data;
    char ch;
    do {
        ch = _getch();
        switch (ch) {
        case 27: data.Esc_controller = true; flag = false; break;
        case 13: data.Enter_controller = true; flag = false; break;
        case 8: if (!data.result.empty()) { data.result.pop_back(); std::cout << "\b \b"; } break;
        case -32: if (flag3) { ch = _getch(); /* стрелки */ }
                else { data.result += ch; if (flag2) std::cout << ch; } break;
        default: data.result += ch; if (flag2) std::cout << ch; break;
        }
    } while (flag);
    return data;
}
