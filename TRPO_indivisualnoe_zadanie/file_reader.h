// Интерфейс чтения файла
#pragma once

#include <string>

// Читает файл целиком; возвращает true при успехе и false при ошибке.
// При успехе записывает данные в content, при ошибке пояснение помещает в error_message.
bool read_text_file(
    const std::string& path,
    std::string& content,
    std::string& error_message);
