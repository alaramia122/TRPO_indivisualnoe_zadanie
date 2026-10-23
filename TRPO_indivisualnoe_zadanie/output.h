// Интерфейс отображения результата
#pragma once

#include "Struct.h"

// Показывает счётчики анализа и подробный список диагностик для пользователя.
void show_analysis_result(
    const HtmlAnalysisResult& result,
    const std::string& source_filename);
