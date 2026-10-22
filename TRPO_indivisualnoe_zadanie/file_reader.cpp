#include "Header.h"

bool read_text_file(
    const std::string& path,
    std::string& content,
    std::string& error_message) {

    std::ifstream file(path, std::ios::binary);

    if (!file.is_open()) {
        error_message = "Не удалось открыть файл: " + path;
        return false;
    }

    std::ostringstream buffer;
    buffer << file.rdbuf();

    if (file.bad()) {
        error_message = "Ошибка при чтении файла: " + path;
        return false;
    }

    content = buffer.str();

    // Удаляем BOM, если файл сохранён с сигнатурой UTF-8.
    if (content.size() >= 3 &&
        static_cast<unsigned char>(content[0]) == 0xEF &&
        static_cast<unsigned char>(content[1]) == 0xBB &&
        static_cast<unsigned char>(content[2]) == 0xBF) {
        content.erase(0, 3);
    }

    error_message.clear();
    return true;
}
