// Интерфейс отображения результата
#pragma once

#include "Struct.h"

// Показывает счётчики анализа и подробный список диагностик для пользователя.
void show_analysis_result(
    const HtmlAnalysisResult& result,
    const std::string& source_filename);

// Сохраняет полный отчёт и возвращает путь либо сообщение об ошибке.
bool save_analysis_report(const HtmlAnalysisResult& result, const std::string& source_filename, std::string& saved_path, std::string& error_message);
