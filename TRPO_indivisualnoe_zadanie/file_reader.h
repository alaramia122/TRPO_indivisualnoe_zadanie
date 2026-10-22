#pragma once

#include <string>

// Читает файл и возвращает статус операции.
bool read_text_file(
    const std::string& path,
    std::string& content,
    std::string& error_message);
