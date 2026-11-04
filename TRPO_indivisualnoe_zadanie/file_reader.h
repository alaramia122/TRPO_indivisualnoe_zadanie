// Интерфейс чтения файла
#pragma once

#include <string>
#include <vector>

// Читает файл построчно и возвращает true при успехе.
// При успехе заполняет lines, при ошибке записывает пояснение в error.
bool read_file_lines(
    const std::string& path,
    std::vector<std::string>& lines,
    std::string& error);