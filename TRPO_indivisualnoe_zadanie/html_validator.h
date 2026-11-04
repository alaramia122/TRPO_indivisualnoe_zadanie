// Интерфейс структурной проверки
#pragma once

#include "Struct.h"

// Проверяет порядок вложенности открывающих и закрывающих тегов.
// Найденные структурные ошибки добавляются в result.diagnostics.
void validate_structure(AnalysisResult& result);