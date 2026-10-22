#include "Header.h"

// Начальный экран объясняет назначение программы и расположение входных HTML-файлов.
void show_start_window() {
    // Очищаем предыдущий экран перед показом заставки.
    system("cls");

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

    // Переходим к следующему экрану после любого нажатия клавиши.
    input_data_reader(true, false, false);
}

// Возвращает код действия; при ошибочном вводе меню повторяет запрос.
int show_main_menu() {
    // Цикл повторяется до выбора допустимого пункта или нажатия Esc.
    while (true) {
        system("cls");

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

        // Esc служит быстрым способом завершить программу из главного меню.
        // При Esc отменяем выбор файла и возвращаем вызывающему коду пустое имя.
        if (input.Esc_controller) {
            return 3;
        }

        // Выбор обрабатывается после Enter и проверки числового формата строки.
        // При незавершённом вводе или посторонних символах запрашиваем номер повторно.
        if (!input.Enter_controller ||
            input.result.empty() ||
            !contains_only(input.result, 'ц')) {
            continue;
        }

        // Возвращаем код первого пункта вызывающему циклу программы.
        if (input.result == "1") {
            return 1;
        }

        // Код второго пункта открывает информационный экран.
        if (input.result == "2") {
            return 2;
        }

        // Код третьего пункта завершает работу программы.
        if (input.result == "3") {
            return 3;
        }
    }
}

// Возвращает имя выбранного HTML-файла; пустая строка обозначает отмену.
std::string choose_source_file() {
    // Повторяем выбор, пока пользователь не укажет существующий файл или не отменит действие.
    while (true) {
        system("cls");

        // Собираем имена файлов отдельно, чтобы отсортировать список перед показом.
        std::vector<std::string> files;

        std::cout
            << std::string(100, '=') << "\n";
        std::cout << "                                             ВЫБОР HTML-ФАЙЛА\n";
        std::cout << "Каталог: SourceTexts\n";

        // Проверяем наличие каталога SourceTexts и корректность его типа.
        if (!fs::exists("SourceTexts") ||
            !fs::is_directory("SourceTexts")) {
            std::cout
                << "Каталог SourceTexts не найден. Создайте его и добавьте HTML-файлы.\n";

            input_data_reader(true, false, false);
            return "";
        }

        // Перебираем содержимое каталога, фильтруя неподходящие объекты и расширения.
        for (const auto& entry : fs::directory_iterator("SourceTexts")) {
            // В список выбора включаем обычные файлы каталога.
            if (!entry.is_regular_file()) {
                continue;
            }

            std::string extension = entry.path().extension().string();

            // Нормализуем регистр расширения, чтобы принимать .html и .htm в любом регистре.
            std::transform(
                extension.begin(),
                extension.end(),
                extension.begin(),
                [](unsigned char ch) {
                    return static_cast<char>(std::tolower(ch));
                });

            // Добавляем только файлы с расширениями, которые поддерживает анализатор.
            if (extension == ".html" || extension == ".htm") {
                files.push_back(entry.path().filename().string());
            }
        }

        // Сортировка сохраняет стабильный и удобный для поиска порядок пунктов.
        std::sort(files.begin(), files.end());

        // При пустом списке подходящих файлов возвращаемся в главное меню.
        if (files.empty()) {
            std::cout
                << "В SourceTexts нет файлов .html или .htm.\n"
                << "Нажмите любую клавишу для возврата...\n";

            input_data_reader(true, false, false);
            return "";
        }

        // Нумерация с единицы совпадает с диапазоном, ожидаемым от пользователя.
        for (std::size_t i = 0; i < files.size(); ++i) {
            std::cout << i + 1 << ". " << files[i] << "\n";
        }

        std::cout << "Введите номер файла (Esc — отмена) > ";

        Input_data input = input_data_reader(true, true, false);

        // Esc отменяет выбор и возвращает пользователя к главному меню.
        if (input.Esc_controller) {
            return "";
        }

        if (!input.Enter_controller ||
            input.result.empty() ||
            !contains_only(input.result, 'ц')) {
            continue;
        }

        // Преобразование строки может выбросить исключение при ошибочном формате или переполнении.
        try {
            // Переводим введённый номер в целое число перед проверкой диапазона.
            const unsigned long choice = std::stoul(input.result);

            // Проверяем границы до индексирования, учитывая нумерацию списка с единицы.
            if (choice >= 1 && choice <= files.size()) {
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
