// utils.h
// Вспомогательные функции для обработки строк и ввода.
#pragma once

#include <string>
#include <vector>
#include "Struct.h"

// Проверка, состоит ли строка только из определённых символов.
bool contains_only(const std::string& str, char flag);

// Режет строку по разделителю.
std::vector<std::string> Cutter_of_string(
    const std::string& stroka,
    char razdel);

// Возвращает первые символы непустых частей строки.
std::vector<char> Cutter_of_string_char(
    const std::string& stroka,
    char razdel);

// Универсальный ввод с клавиатуры.
Input_data input_data_reader(
    bool loop_until_enter,
    bool echo_input = true,
    bool allow_arrows = false);
