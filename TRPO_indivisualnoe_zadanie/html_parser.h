#pragma once // Защищаем интерфейс парсера.
#include "Struct.h" // Подключаем структуры токенов.
#include <string> // Подключаем исходный текст.
void parse_html(const std::string& source, HtmlAnalysisResult& result); // Разбираем HTML и заполняем результат.
