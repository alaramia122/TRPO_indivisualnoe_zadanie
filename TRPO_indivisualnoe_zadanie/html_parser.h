#pragma once

#include <string>
#include "Struct.h"

// Разбирает HTML и заполняет результат анализа.
void parse_html(
    const std::string& source,
    HtmlAnalysisResult& result);
