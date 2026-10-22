#pragma once // Защищаем объявления модуля.
#include <string> // Используем строковые пути и содержимое.
bool read_text_file(const std::string& path, std::string& content, std::string& error_message); // Читаем файл и возвращаем статус.
