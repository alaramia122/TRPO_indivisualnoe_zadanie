// utils.h
// Вспомогательные функции для обработки строк и клавиатурного ввода.
#pragma once

#include <string>
#include <vector>
#include "Struct.h"

// Проверяет, соответствует ли каждый символ строки набору, выбранному через flag.
// Возвращает true только при успешной проверке всех символов.
bool contains_only(const std::string& str, char flag);

// Разбивает строку по символу-разделителю и возвращает полученные фрагменты.
std::vector<std::string> Cutter_of_string(
    const std::string& stroka,
    char razdel);

// Возвращает первый символ каждого непустого фрагмента строки.
std::vector<char> Cutter_of_string_char(
    const std::string& stroka,
    char razdel);

// Считывает клавиши до Enter, Esc или стрелки и возвращает строку вместе с состоянием управляющих клавиш.
// flag задаёт продолжение чтения, flag2 — отображение ввода,
// flag3 — специальную обработку расширенных клавиш (стрелки, Delete).
Input_data input_data_reader(
    bool flag,
    bool flag2 = true,
    bool flag3 = false);