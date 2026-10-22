#include "Header.h" // Подключаем общие объявления.
bool read_text_file(const std::string& path, std::string& content, std::string& error_message) { // Загружаем текстовый файл.
    std::ifstream file(path, std::ios::binary); // Открываем файл без преобразования переводов строк.
    if (!file.is_open()) { error_message = "Не удалось открыть файл: " + path; return false; } // Возвращаем ошибку открытия.
    std::ostringstream buffer; // Создаём буфер для содержимого.
    buffer << file.rdbuf(); // Считываем весь файл в буфер.
    if (file.bad()) { error_message = "Ошибка при чтении файла: " + path; return false; } // Проверяем ошибку чтения.
    content = buffer.str(); // Сохраняем считанное содержимое.
    if (content.size() >= 3 && static_cast<unsigned char>(content[0]) == 0xEF && static_cast<unsigned char>(content[1]) == 0xBB && static_cast<unsigned char>(content[2]) == 0xBF) content.erase(0, 3); // Удаляем UTF-8 BOM.
    error_message.clear(); // Очищаем сообщение об ошибке.
    return true; // Подтверждаем успешное чтение.
}
