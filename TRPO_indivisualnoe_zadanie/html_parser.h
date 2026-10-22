// Защищаем интерфейс парсера.
#pragma once

// Подключаем структуры токенов.
#include "Struct.h"

// Подключаем исходный текст.
#include <string>

// Разбираем HTML и заполняем результат.
void parse_html(
    const std::string& source,
    HtmlAnalysisResult& result);
