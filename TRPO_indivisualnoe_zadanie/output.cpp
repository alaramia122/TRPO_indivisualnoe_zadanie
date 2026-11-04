#include "Header.h"

// ---------------------------------------------------------------------------
// Формирование строк и разделов по типам токенов
// ---------------------------------------------------------------------------

// Собирает одну читаемую строку описания токена для экрана и текстового отчёта.
std::string format_token(const Token& token) {

    switch (token.type) {                                                   // Формат строки зависит от категории токена.

    case MyTokenType::Open:                                                     // Открывающий тег: имя и список атрибутов.
    case MyTokenType::SelfClose: {                                              // Самозакрывающийся тег отличается только завершающим слешем.
        std::string line = "<" + token.name;                                        // Начинаем строку с имени тега в угловых скобках.

        for (const Attribute& attribute : token.attrs) {                            // Дописываем атрибуты в том виде, как их собрал парсер.
            line += " ";
            line += attribute.name;
            if (!attribute.value.empty()) {                                             // Пустое значение не оформляем как name="".
                line += "=\"";
                line += attribute.value;
                line += "\"";
            }
        }

        if (token.type == MyTokenType::SelfClose) {                                 // Для самозакрывающегося тега показываем завершающий слеш.
            line += " /";
        }

        line += ">";                                                                // Закрываем визуальное представление тега.
        line += "  (стр. ";                                                         // Добавляем номер строки исходного файла.
        line += std::to_string(token.line);
        line += ")";
        return line;
    }

    case MyTokenType::Close:                                                    // Закрывающий тег показываем в виде </name>.
		return "</" + token.name + ">  (стр. " + std::to_string(token.line) + ")";  // Закрывающий тег не имеет атрибутов.

    case MyTokenType::Comment:                                                  // Комментарий выводим исходным текстом токена.
    case MyTokenType::Text:                                                     // Текстовый фрагмент выводим без дополнительной очистки.
    case MyTokenType::Other:                                                    // Декларации и специальные конструкции тоже как исходный текст.
		return token.text + "  (стр. " + std::to_string(token.line) + ")";          // Для этих категорий токенов текстовое представление уже готово.
    }

    return token.text;                                                      // На случай расширения enum оставляем запасной путь.
}

// Если в разделе нет элементов, оставляем одну заглушку, чтобы раздел можно было открыть стрелками.
void ensure_not_empty(std::vector<std::string>& values) {

	if (values.empty()) {                                                   // Если раздел пуст
		values.push_back("Список пуст.");                                       // Добавляем заглушку
    }
}

// Строит разделы интерфейса: один раздел на каждый тип токена.
std::vector<std::pair<std::string, std::vector<std::string>>>
make_sections(const AnalysisResult& result) {

    std::vector<std::string> open_values;                                   // Описания всех открывающих тегов.
    std::vector<std::string> close_values;                                  // Описания всех закрывающих тегов.
    std::vector<std::string> self_values;                                   // Описания всех самозакрывающихся тегов.
    std::vector<std::string> comment_values;                                // Описания комментариев.
    std::vector<std::string> text_values;                                   // Описания текстовых фрагментов.
    std::vector<std::string> other_values;                                  // Описания деклараций и прочих конструкций.

    for (const Token& token : result.tokens) {                              // Раскладываем токены по корзинам в порядке появления в документе.

		switch (token.type) {                                                   // Формат строки зависит от категории токена.
        case MyTokenType::Open:
			open_values.push_back(format_token(token));                                 // Формируем строку описания открывающего тега.
            break;
        case MyTokenType::Close:
			close_values.push_back(format_token(token));                                // Формируем строку описания закрывающего тега.
            break;
        case MyTokenType::SelfClose:
			self_values.push_back(format_token(token));                                 // Формируем строку описания самозакрывающегося тега.
            break;
        case MyTokenType::Comment:
			comment_values.push_back(format_token(token));                              // Формируем строку описания комментария.
            break;
        case MyTokenType::Text:
			text_values.push_back(format_token(token));                                 // Формируем строку описания текстового фрагмента.
            break;
        case MyTokenType::Other:
			other_values.push_back(format_token(token));                                // Формируем строку описания декларации или специальной конструкции.
            break;
        }
    }

    ensure_not_empty(open_values);                                          // Пустой раздел всё равно должен отображаться в навигации.
    ensure_not_empty(close_values);
    ensure_not_empty(self_values);
    ensure_not_empty(comment_values);
    ensure_not_empty(text_values);
    ensure_not_empty(other_values);

    std::vector<std::pair<std::string, std::vector<std::string>>> sections;  // Итоговый список разделов в фиксированном порядке.
    sections.push_back({ "Открывающие теги", open_values });
    sections.push_back({ "Закрывающие теги", close_values });
    sections.push_back({ "Самозакрывающиеся теги", self_values });
    sections.push_back({ "Комментарии", comment_values });
    sections.push_back({ "Текст", text_values });
    sections.push_back({ "Прочее", other_values });
    return sections;
}

// ---------------------------------------------------------------------------
// Экран статистики и диагностик
// ---------------------------------------------------------------------------

// Выводит счётчики, накопленные на этапе лексического разбора.
void print_statistics(const AnalysisResult& result) {

    std::cout << std::string(100, '=') << "\n";                             // Верхняя рамка экрана.
    std::cout << "                                          РЕЗУЛЬТАТ АНАЛИЗА HTML\n";
    std::cout << std::string(100, '=') << "\n";
    std::cout << "Открывающие теги:       " << result.open_tags << "\n";
    std::cout << "Закрывающие теги:       " << result.close_tags << "\n";
    std::cout << "Самозакрывающиеся теги: " << result.self_tags << "\n";
    std::cout << "Атрибуты:               " << result.attributes << "\n";
    std::cout << "Комментарии:            " << result.comments << "\n";
    std::cout << "Текстовые фрагменты:    " << result.texts << "\n";
    std::cout << "Диагностики:            " << result.diagnostics.size() << "\n";
}

// Выводит диагностики в порядке их обнаружения парсером и валидатором.
void print_diagnostics(const AnalysisResult& result) {

    std::cout << "\nДИАГНОСТИКИ\n";
    std::cout << std::string(60, '-') << "\n";

    if (result.diagnostics.empty()) {                                       // Пустой список считаем успешным завершением проверки.
        std::cout
            << "Проверка поддерживаемого подмножества HTML завершена успешно.\n";
        return;
    }

    for (const Diagnostic& item : result.diagnostics) {                     // Печатаем категорию, номер строки и текст сообщения.
        std::cout << "[" << item.category << "] строка "
            << item.line << ": " << item.message << "\n";
    }
}

// ---------------------------------------------------------------------------
// Навигация по разделам стрелками
// ---------------------------------------------------------------------------

// Позволяет листать типы токенов и отдельные элементы внутри выбранного типа.
void browse_sections(
    const std::vector<std::pair<std::string, std::vector<std::string>>>& sections) {

    if (sections.empty()) {                                                 // Защита на случай пустого списка разделов.
        return;
    }

    std::size_t section_index = 0;                                          // Индекс текущего раздела (типа токенов).
    std::size_t value_index = 0;                                            // Индекс текущего элемента внутри раздела.

    while (true) {                                                          // Цикл продолжается до Enter или Esc.

        system("cls");                                                          // Обновляем экран перед каждой перерисовкой.

        const std::string& section_name = sections[section_index].first;        // Название текущего типа токенов.
        const std::vector<std::string>& values =
            sections[section_index].second;                                     // Все элементы выбранного типа.

        std::cout << "ТОКЕНЫ ПО ТИПАМ\n";
        std::cout << "Раздел " << (section_index + 1) << " из "
            << sections.size() << ": " << section_name << "\n";
        std::cout << "Элемент " << (value_index + 1) << " из "
            << values.size() << "\n\n";
        std::cout << "  " << values[value_index] << "\n\n";                     // Текущий элемент раздела.
        std::cout << "Стрелки вверх/вниз — тип токенов.\n";
        std::cout << "Стрелки влево/вправо — элемент внутри типа.\n";
        std::cout << "Enter или Esc — дальше.\n";

        Input_data key = input_data_reader(true, false, true);                  // Читаем одно нажатие, включая стрелки.

        if (key.Enter_controller || key.Esc_controller) {                       // Пользователь закончил просмотр токенов.
            break;
        }

        if (key.Up_controller) {                                                // Переход к предыдущему типу токенов.
			if (section_index == 0) {                                               // Если текущий раздел первый
                section_index = sections.size() - 1;                                    // С первого раздела переходим к последнему.
            }
            else {                                                                  // Иначе
				--section_index;                                                        // Переходим к предыдущему разделу.
            }
            value_index = 0;                                                        // После смены раздела начинаем с первого элемента.
        }
        else if (key.Down_controller) {                                         // Переход к следующему типу токенов.
            ++section_index;
			if (section_index >= sections.size()) {                                 // Если текущий раздел последний
                section_index = 0;                                                      // Возвращаемся к первому.
            }
			value_index = 0;                                                        // После смены раздела начинаем с первого элемента.
        }
        else if (key.Left_controller) {                                         // Предыдущий элемент внутри текущего типа.
			if (value_index == 0) {                                                 // Если текущий элемент первый
				value_index = values.size() - 1;                                        // Переходим к последнему элементу.
            }
			else {                                                                  // Иначе
				--value_index;                                                          // Переходим к предыдущему элементу.
            }
        }
        else if (key.Right_controller) {                                        // Следующий элемент внутри текущего типа.
			++value_index;                                                          // Переходим к следующему элементу.
			if (value_index >= values.size()) {                                         // Если текущий элемент последний
				value_index = 0;                                                            // Возвращаемся к первому элементу.
            }
        }
    }
}

// ---------------------------------------------------------------------------
// Сохранение отчёта
// ---------------------------------------------------------------------------

// Записывает статистику, токены по типам и диагностики в каталог AnalysisResults.
bool save_report(
    const AnalysisResult& result,
    const std::string& source_filename,
    const std::vector<std::pair<std::string, std::vector<std::string>>>& sections,
    std::string& saved_path,                                                // Сюда возвращается путь созданного файла.
    std::string& error_message) {                                           // Сюда возвращается текст ошибки при неудаче.

    try {
        fs::create_directories("AnalysisResults");                          // Создаём каталог отчётов, если его ещё нет.

        std::string stem = source_filename;                                 // Базовое имя строим из имени исходного файла.
        const std::size_t dot = stem.find_last_of('.');
        if (dot != std::string::npos) {                                     // Убираем расширение, если оно присутствует.
            stem = stem.substr(0, dot);
        }

        const std::string base = stem + "_analysis";                        // Общая часть имени отчёта.
        fs::path path = fs::path("AnalysisResults") / (base + ".txt");

        std::size_t suffix = 2;                                             // Если файл уже существует, добавляем числовой суффикс.
        while (fs::exists(path)) {
            path = fs::path("AnalysisResults") /
                (base + "_" + std::to_string(suffix) + ".txt");
            ++suffix;
        }

        std::ofstream file(path.string());                                  // Пишем обычный текст в системной кодировке (CP1251).
        if (!file.is_open()) {
            error_message = "Не удалось открыть файл для записи.";
            return false;
        }

        file << "РЕЗУЛЬТАТ АНАЛИЗА HTML\n";
        file << std::string(60, '=') << "\n\n";
        file << "Исходный файл: " << source_filename << "\n\n";

        file << "СТАТИСТИКА\n";
        file << std::string(40, '-') << "\n";
        file << "Открывающие теги:       " << result.open_tags << "\n";
        file << "Закрывающие теги:       " << result.close_tags << "\n";
        file << "Самозакрывающиеся теги: " << result.self_tags << "\n";
        file << "Атрибуты:               " << result.attributes << "\n";
        file << "Комментарии:            " << result.comments << "\n";
        file << "Текстовые фрагменты:    " << result.texts << "\n";
        file << "Диагностики:            " << result.diagnostics.size() << "\n\n";

        file << "ТОКЕНЫ ПО ТИПАМ\n";
        file << std::string(40, '-') << "\n";
        for (std::size_t i = 0; i < sections.size(); ++i) {                 // Пишем разделы в том же порядке, что и на экране.
            file << sections[i].first << ":\n";
            for (std::size_t j = 0; j < sections[i].second.size(); ++j) {
                file << "  - " << sections[i].second[j] << "\n";
            }
            file << "\n";
        }

        file << "ДИАГНОСТИКИ\n";
        file << std::string(40, '-') << "\n";
        if (result.diagnostics.empty()) {
            file << "Проверка поддерживаемого подмножества HTML завершена успешно.\n";
        }
        else {
            for (const Diagnostic& item : result.diagnostics) {
                file << "[" << item.category << "] строка "
                    << item.line << ": " << item.message << "\n";
            }
        }

        file.close();                                                       // Явно закрываем файл после записи.
        saved_path = path.string();                                         // Сообщаем вызывающему коду путь к отчёту.
        error_message.clear();
        return true;
    }
    catch (const std::exception& ex) {                                      // Ошибки файловой системы не должны завершать всю программу.
        error_message = std::string("Ошибка при сохранении: ") + ex.what();
        return false;
    }
}

// Спрашивает пользователя, нужно ли сохранить отчёт, и при согласии записывает файл.
void ask_save(
    const AnalysisResult& result,
    const std::string& source_filename,
    const std::vector<std::pair<std::string, std::vector<std::string>>>& sections) {

    while (true) {                                                          // Повторяем запрос, пока не будет дан корректный ответ.

        system("cls");
        std::cout << std::string(100, '=') << "\n";
        std::cout << "                                      СОХРАНЕНИЕ РЕЗУЛЬТАТА\n";
        std::cout << std::string(100, '=') << "\n";
        std::cout << "Сохранить отчёт в AnalysisResults?\n";
        std::cout << "1 — да\n";
        std::cout << "0 — нет\n";
        std::cout << "Ваш выбор > ";

        Input_data input = input_data_reader(true, true, false);            // Ждём ввод номера пункта меню.

        if (input.Esc_controller) {                                         // Esc означает отказ от сохранения.
            return;
        }

        if (!input.Enter_controller ||                                      // Обрабатываем только подтверждённый числовой ввод.
            input.result.empty() ||
            !contains_only(input.result, 'd')) {
            continue;
        }

        if (input.result == "0") {                                          // Пользователь отказался сохранять отчёт.
            return;
        }

        if (input.result == "1") {                                          // Пользователь подтвердил сохранение.
            std::string saved_path;
            std::string error_message;

            if (save_report(result, source_filename, sections, saved_path, error_message)) {
                std::cout << "\nОтчёт сохранён: " << saved_path << "\n";
            }
            else {
                std::cout << "\n" << error_message << "\n";
            }

            std::cout << "Нажмите любую клавишу...\n";
            input_data_reader(true, false, false);
            return;
        }
    }
}

// ---------------------------------------------------------------------------
// Точка входа модуля
// ---------------------------------------------------------------------------

// Последовательно показывает статистику, токены по типам, диагностики и диалог сохранения.
void show_analysis_result(
    const AnalysisResult& result,
    const std::string& source_filename) {

    system("cls");                                                          // Начинаем с экрана сводной статистики.
    print_statistics(result);
    std::cout << "\nНажмите любую клавишу для просмотра токенов...\n";
    input_data_reader(true, false, false);

    std::vector<std::pair<std::string, std::vector<std::string>>> sections =
        make_sections(result);                                              // Готовим разделы по типам токенов.

    browse_sections(sections);                                              // Пользователь листает типы и элементы стрелками.

    system("cls");                                                          // После токенов показываем диагностики.
    print_statistics(result);
    print_diagnostics(result);
    std::cout << "\nНажмите любую клавишу для продолжения...\n";
    input_data_reader(true, false, false);

    ask_save(result, source_filename, sections);                            // В конце предлагаем сохранить полный отчёт.
}