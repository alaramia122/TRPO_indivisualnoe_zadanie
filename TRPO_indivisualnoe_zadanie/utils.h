#pragma once

#include <string>
#include <vector>
#include "Struct.h"

// Проверяет, содержит ли строка только допустимые символы.
bool contains_only(const std::string& str, char flag);

// Разделяет строку по заданному символу.
std::vector<std::string> Cutter_of_string(
    const std::string& stroka,
    char razdel);

// Возвращает первые символы непустых частей строки.
std::vector<char> Cutter_of_string_char(
    const std::string& stroka,
    char razdel);

// Считывает ввод с обработкой Enter и Esc.
Input_data input_data_reader(
    bool loop_until_enter,
    bool echo_input = true,
    bool allow_arrows = false);
