#include "Header.h"

bool read_text_file(
    const std::string& path,
    std::string& content,
    std::string& error_message) {

    // Двоичный режим сохраняет исходные байты и не преобразует переводы строк.
    std::ifstream file(path, std::ios::binary);

    // Без открытого потока продолжать чтение нельзя — возвращаем диагностическое сообщение.
    if (!file.is_open()) {
        error_message = "Не удалось открыть файл: " + path;
        return false;
    }

    // Буфер накапливает содержимое файла до передачи его в строку.
    std::ostringstream buffer;
    // Копируем весь файловый поток, не разбирая HTML на этом этапе.
    buffer << file.rdbuf();

    // Проверяем низкоуровневую ошибку чтения после копирования потока.
    if (file.bad()) {
        error_message = "Ошибка при чтении файла: " + path;
        return false;
    }

    // Публикуем прочитанный текст только после проверки состояния потока.
    content = buffer.str();

    // Удаляем BOM, если файл сохранён с сигнатурой UTF-8.
    if (content.size() >= 3 &&
        static_cast<unsigned char>(content[0]) == 0xEF &&
        static_cast<unsigned char>(content[1]) == 0xBB &&
        static_cast<unsigned char>(content[2]) == 0xBF) {
        content.erase(0, 3);
    }

    // Успешное чтение означает, что старое сообщение об ошибке больше не актуально.
    error_message.clear();
    return true;
}
