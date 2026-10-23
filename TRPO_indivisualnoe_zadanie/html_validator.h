// Интерфейс структурной проверки
#pragma once

// Валидатор читает токены и добавляет найденные проблемы в общий результат.
#include "Struct.h"

// Проверяет порядок вложенности открывающих и закрывающих тегов.
// Найденные структурные ошибки добавляются в result.diagnostics.
void validate_html_structure(HtmlAnalysisResult& result);
