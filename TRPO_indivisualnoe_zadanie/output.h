// Интер// Интерфейс отображения результата
#pragma once

#include "Struct.h"
#include <string>

// Показывает статистику, сведения из документа, диагностики и предлагает сохранение.
void show_analysis_result(
    AnalysisResult& result,
    std::string& source_filename);