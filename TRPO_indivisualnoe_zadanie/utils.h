// utils.h
// Вспомогательные функции для обработки строк и ввода.
#pragma once

// Строки нужны для ввода и обработки текстовых данных.
#include <string>
// Векторы используются для возвращаемых списков фрагментов.
#include <vector>
// Подключаем структуру результата клавиатурного ввода.
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

// Считывает клавиши до Enter/Esc и возвращает строку вместе с состоянием управляющих клавиш.
// loop_until_enter задаёт продолжение чтения, echo_input — отображение ввода,
// allow_arrows — специальную обработку расширенных клавиш.
Input_data input_data_reader(
    bool loop_until_enter,
    bool echo_input = true,
    bool allow_arrows = false);
