#include "Header.h" // Подключаем общие объявления приложения.
int main() { // Начинаем выполнение программы.
    show_start_window(); // Показываем стартовый экран.
    while (true) { // Повторяем действия пользователя до выхода.
        const int choice = show_main_menu(); // Получаем пункт главного меню.
        if (choice == 1) { // Запускаем проверку файла.
            const std::string filename = choose_source_file(); // Получаем выбранное имя файла.
            if (filename.empty()) continue; // Возвращаемся в меню при отмене или отсутствии файлов.
            const std::string path = (fs::path("SourceTexts") / filename).string(); // Формируем путь к файлу.
            std::string source, error_message; // Подготавливаем исходный текст и сообщение об ошибке.
            if (!read_text_file(path, source, error_message)) { system("cls"); std::cout << error_message << "\nНажмите любую клавишу для возврата...\n"; input_data_reader(true, false, false); continue; } // Обрабатываем ошибку чтения и возвращаемся в меню.
            HtmlAnalysisResult result; // Создаём отдельный результат для текущего файла.
            parse_html(source, result); // Разбираем HTML вручную.
            validate_html_structure(result); // Проверяем вложенность тегов.
            show_analysis_result(result); // Выводим статистику и диагностики.
        } else if (choice == 2) show_about_screen(); // Показываем информацию о программе.
        else break; // Завершаем цикл при выборе выхода.
    } // Завершаем основной цикл.
    return 0; // Возвращаем код успешного завершения.
} // Завершаем точку входа.
