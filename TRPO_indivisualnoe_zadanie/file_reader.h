// Защищаем объявления модуля.
#pragma once

// Используем строковые пути и содержимое.
#include <string>

// Читаем файл и возвращаем статус.
bool read_text_file(
    const std::string& path,
    std::string& content,
    std::string& error_message);
