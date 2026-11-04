// Интерфейс лексического разбора
#pragma once

#include "Struct.h"
#include <string>
#include <vector>

// Разбирает исходные строки и заполняет токены, счётчики и диагностики.
// Структурная согласованность парных тегов проверяется отдельным модулем.
void parse_html(
    const std::vector<std::string>& lines,
    AnalysisResult& result);