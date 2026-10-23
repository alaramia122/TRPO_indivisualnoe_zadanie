#include "Header.h"

// Начальный экран объясняет назначение программы и расположение входных HTML-файлов.
void show_start_window() {
    
    system("cls");                                                                                      // Очищаем предыдущий экран перед показом заставки.

    std::cout
        << "\n" << std::string(100, '=') << "\n";
    std::cout
        << "                                      АНАЛИЗАТОР СТРУКТУРЫ HTML-ДОКУМЕНТОВ\n";
    std::cout
        << "                                      Индивидуальная лабораторная работа по ТРПО\n";
    std::cout
        << std::string(100, '=') << "\n";
    std::cout << "Входные файлы: SourceTexts (.html, .htm)\n";
    std::cout
        << "Проверка выполняется для поддерживаемого подмножества HTML.\n\n";
    std::cout << "Нажмите любую клавишу для продолжения...\n";

    
    input_data_reader(true, false, false);                                                              // Переходим к следующему экрану после любого нажатия клавиши.
}

// Возвращает код действия; при ошибочном вводе меню повторяет запрос.
int show_main_menu() {
    
    while (true) {                                                                                      // Цикл повторяется до выбора допустимого пункта или нажатия Esc.
        system("cls");                                                                                      // Очищаем предыдущий экран перед показом страницы.

        std::cout
            << std::string(100, '=') << "\n";
        std::cout << "                                             ГЛАВНОЕ МЕНЮ\n";
        std::cout
            << std::string(100, '=') << "\n";
        std::cout << "1. Проверить HTML-файл\n";
        std::cout << "2. Информация о программе\n";
        std::cout << "3. Выход\n";
        std::cout
            << std::string(100, '=') << "\n";
        std::cout << "Ваш выбор > ";

                                                                                                            // Считываем введённую строку и состояние клавиш Enter/Esc.
                                                                                                            // Получаем номер пункта или сигнал отмены выбора.
        Input_data input = input_data_reader(true, true, false);
                                   
        if (input.Esc_controller) {                                                                         // Esc служит быстрым способом завершить программу из главного меню.
            return 3;                                                                                       // При Esc отменяем выбор файла и возвращаем вызывающему коду пустое имя.
        }

        if (!input.Enter_controller ||                                                                      // Выбор обрабатывается после Enter и проверки числового формата строки.
            input.result.empty() ||                                                                         // При вводе с посторонними символами повторяем запрос.
            !contains_only(input.result, 'ц')) {
            continue;
        }
        
        if (input.result == "1") {                                                                          // Возвращаем код первого пункта вызывающему циклу программы.
            return 1;
        }
        
        if (input.result == "2") {                                                                          // Код второго пункта открывает информационный экран.
            return 2;
        }

        if (input.result == "3") {                                                                          // Код третьего пункта завершает работу программы.
            return 3;
        }
    }
}

// Возвращает имя выбранного HTML-файла; пустая строка обозначает отмену.
std::string choose_source_file() {
    
    while (true) {                                                                                      // Повторяем выбор до указания существующего файла или отмены действия.
        system("cls");                                                                                      // Очищаем предыдущий экран перед показом страницы.

        std::vector<std::string> files;                                                                     // Собираем имена файлов отдельно, чтобы отсортировать список перед показом.

        std::cout
            << std::string(100, '=') << "\n";
        std::cout << "                                             ВЫБОР HTML-ФАЙЛА\n";
        std::cout << "Каталог: SourceTexts\n";

        
        if (!fs::exists("SourceTexts") ||                                                                   // Проверяем наличие каталога SourceTexts и корректность его типа.
            !fs::is_directory("SourceTexts")) {
            std::cout
                << "Каталог SourceTexts: проверьте путь и добавьте HTML-файлы.\n";

            input_data_reader(true, false, false);
            return "";
        }

        
        for (const auto& entry : fs::directory_iterator("SourceTexts")) {                                   // Перебираем содержимое каталога и отбираем обычные HTML-файлы.
            if (!entry.is_regular_file()) {                                                                     // В список выбора включаем обычные файлы каталога.
                continue;
            }

            std::string extension = entry.path().extension().string();
            
            std::transform(                                                                                     // Нормализуем регистр расширения, чтобы принимать .html и .htm в любом регистре.
                extension.begin(),
                extension.end(),
                extension.begin(),
                [](unsigned char ch) {
                    return static_cast<char>(std::tolower(ch));
                });

            if (extension == ".html" || extension == ".htm") {                                                  // Добавляем только файлы с расширениями, которые поддерживает анализатор.
                files.push_back(entry.path().filename().string());
            }
        }

        std::sort(files.begin(), files.end());                                                              // Сортировка сохраняет стабильный и удобный для поиска порядок пунктов.

        if (files.empty()) {                                                                                // При пустом списке подходящих файлов возвращаемся в главное меню.
            std::cout
                << "Добавьте в SourceTexts файлы с расширением .html или .htm.\n"
                << "Нажмите любую клавишу для возврата...\n";

            input_data_reader(true, false, false);
            return "";
        }

        for (std::size_t i = 0; i < files.size(); ++i) {                                                    // Нумерация с единицы совпадает с диапазоном, ожидаемым от пользователя.
            std::cout << i + 1 << ". " << files[i] << "\n";
        }

        std::cout << "Введите номер файла (Esc — отмена) > ";

        Input_data input = input_data_reader(true, true, false);

        
        if (input.Esc_controller) {                                                                         // Esc отменяет выбор и возвращает пользователя к главному меню.
            return "";
        }

        if (!input.Enter_controller ||
            input.result.empty() ||
            !contains_only(input.result, 'ц')) {
            continue;
        }

        
        try {                                                                                               // Преобразование строки может выбросить исключение при ошибочном формате или переполнении.
           
            const unsigned long choice = std::stoul(input.result);                                              // Переводим введённый номер в целое число перед проверкой диапазона.

            if (choice >= 1 && choice <= files.size()) {                                                        // Проверяем границы до индексирования, учитывая нумерацию списка с единицы.
                return files[choice - 1];
            }
        } catch (const std::exception&) {
            // При ошибочном номере цикл повторно показывает приглашение.

            // При ошибочном номере повторяем запрос.
        }
    }
}

// Показываем пользователю возможности программы и границы поддерживаемой проверки HTML.
void show_about_screen() {
    system("cls");

    std::cout
        << std::string(100, '=') << "\n";
    std::cout << "                                         ИНФОРМАЦИЯ О ПРОГРАММЕ\n";
    std::cout
        << "Разбираются открывающие, закрывающие и самозакрывающиеся теги, атрибуты, комментарии и текст.\n";
    std::cout
        << "Вложенность парных тегов проверяется стековым алгоритмом.\n";
    std::cout
        << "Поддерживается подмножество HTML с базовым разбором атрибутов и структуры тегов.\n";
    std::cout
        << "Значения атрибутов поддерживают одинарные и двойные кавычки, а также свободную запись.\n";
    std::cout
        << "Одиночные элементы: area, base, br, col, embed, hr, img, input, link, meta, param, source, track, wbr.\n";
    std::cout
        << "Структурная модель ориентирована на явные закрывающие теги.\n";
    std::cout
        << "Правила HTML5 для опциональных закрывающих тегов требуют отдельной валидации.\n";
    std::cout << "Нажмите любую клавишу для возврата...\n";

    input_data_reader(true, false, false);
}

// Выводим сводку завершённой операции, если этот экран вызывается из сценария работы.
void show_completion_screen(
    const std::string& filename,
    std::size_t errors) {
    std::cout << "\nФайл: " << filename
              << "\nКоличество диагностик: " << errors << "\n";
    std::cout
        << "Нажмите любую клавишу для возврата в главное меню...\n";

    input_data_reader(true, false, false);
}
