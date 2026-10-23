// Интерфейс лексического разбора
#pragma once

#include "Struct.h"
#include <string>

// Разбирает исходный текст и добавляет распознанные токены, счётчики и диагностики.
// Структурная согласованность парных тегов проверяется отдельным модулем.
void parse_html(
    const std::string& source,
    HtmlAnalysisResult& result);
