// Интерфейс отображения результата
#pragma once

#include "Struct.h"
#include <string>

// Показывает счётчики анализа, извлечённые сведения и список диагностик.
void show_analysis_result(
    const AnalysisResult& result,
    const std::string& source_filename);