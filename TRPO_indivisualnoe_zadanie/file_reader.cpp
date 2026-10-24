#include "Header.h"

bool read_text_file(
    const std::string& path,
    std::string& content,
    std::string& error_message) {

    std::ifstream file(fs::u8path(path), std::ios::binary);     // Двоичный режим сохраняет исходные байты и исходные переводы строк. Важно для корректного прочтения HTML.

    if (!file.is_open()) {                                      // При ошибке открытия возвращаем диагностическое сообщение вызывающему коду.
        error_message = "Ошибка открытия файла: " + path;
        return false;
    }

    std::ostringstream buffer;                                  // Буфер накапливает содержимое файла до передачи его в строку.
    
    buffer << file.rdbuf();                                     // На этом этапе копируем весь файловый поток в исходную строку.

    if (file.bad()) {                                           // Проверяем низкоуровневую ошибку чтения после копирования потока.
        error_message = "Ошибка при чтении файла: " + path;
        return false;
    }

    content = buffer.str();                                     // Публикуем прочитанный текст после проверки состояния потока.

    if (content.size() >= 3 &&                                  // Удаляем BOM (Byte Order Mark), если файл сохранён с сигнатурой UTF-8.
        static_cast<unsigned char>(content[0]) == 0xEF &&
        static_cast<unsigned char>(content[1]) == 0xBB &&
        static_cast<unsigned char>(content[2]) == 0xBF) {
        content.erase(0, 3);
    }

    error_message.clear();                                      // После успешного чтения очищаем прежнее сообщение об ошибке.
    return true;
}
