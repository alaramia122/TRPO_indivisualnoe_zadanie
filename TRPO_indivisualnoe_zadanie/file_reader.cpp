#include "Header.h"

// Открываем файл и последовательно считываем каждую строку в вектор.
bool read_file_lines(
    const std::string& path,
    std::vector<std::string>& lines,
    std::string& error) {

    std::ifstream file(path);                                               // Открываем файл в текстовом режиме по указанному пути.

    if (!file.is_open()) {                                                  // При ошибке открытия возвращаем диагностическое сообщение вызывающему коду.
        error = "Не удалось открыть файл: " + path;
        return false;
    }

    lines.clear();                                                          // Очищаем прежнее содержимое, чтобы результат содержал только строки текущего файла.
    std::string line;

    while (std::getline(file, line)) {                                      // Читаем файл построчно до исчерпания потока.
        lines.push_back(line);                                              // Каждая прочитанная строка сохраняется без дополнительной обработки.
    }

    error.clear();                                                          // После успешного чтения очищаем прежнее сообщение об ошибке.
    return true;
}