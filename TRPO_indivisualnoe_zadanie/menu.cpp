#include "Header.h"

void show_start_window() {
    system("cls");

    std::cout
        << "\n=======================================================================================================================\n";
    std::cout
        << "                                      АНАЛИЗАТОР СТРУКТУРЫ HTML-ДОКУМЕНТОВ\n";
    std::cout
        << "                                      Индивидуальная лабораторная работа по ТРПО\n";
    std::cout
        << "=======================================================================================================================\n";
    std::cout << "Входные файлы: SourceTexts (.html, .htm)\n";
    std::cout
        << "Проверка выполняется для поддерживаемого подмножества HTML.\n\n";
    std::cout << "Нажмите любую клавишу для продолжения...\n";

    input_data_reader(true, false, false);
}

int show_main_menu() {
    while (true) {
        system("cls");

        std::cout
            << "=======================================================================================================================\n";
        std::cout << "                                             ГЛАВНОЕ МЕНЮ\n";
        std::cout
            << "=======================================================================================================================\n";
        std::cout << "1. Проверить HTML-файл\n";
        std::cout << "2. Информация о программе\n";
        std::cout << "3. Выход\n";
        std::cout
            << "=======================================================================================================================\n";
        std::cout << "Ваш выбор > ";

        Input_data input = input_data_reader(true, true, false);

        if (input.Esc_controller) {
            return 3;
        }

        if (!input.Enter_controller ||
            input.result.empty() ||
            !contains_only(input.result, 'ц')) {
            continue;
        }

        if (input.result == "1") {
            return 1;
        }

        if (input.result == "2") {
            return 2;
        }

        if (input.result == "3") {
            return 3;
        }
    }
}

std::string choose_source_file() {
    while (true) {
        system("cls");

        std::vector<std::string> files;

        std::cout
            << "=======================================================================================================================\n";
        std::cout << "                                             ВЫБОР HTML-ФАЙЛА\n";
        std::cout << "Каталог: SourceTexts\n";

        if (!fs::exists("SourceTexts") ||
            !fs::is_directory("SourceTexts")) {
            std::cout
                << "Каталог SourceTexts не найден. Создайте его и добавьте HTML-файлы.\n";

            input_data_reader(true, false, false);
            return "";
        }

        for (const auto& entry : fs::directory_iterator("SourceTexts")) {
            if (!entry.is_regular_file()) {
                continue;
            }

            std::string extension = entry.path().extension().string();

            std::transform(
                extension.begin(),
                extension.end(),
                extension.begin(),
                [](unsigned char ch) {
                    return static_cast<char>(std::tolower(ch));
                });

            if (extension == ".html" || extension == ".htm") {
                files.push_back(entry.path().filename().string());
            }
        }

        std::sort(files.begin(), files.end());

        if (files.empty()) {
            std::cout
                << "В SourceTexts нет файлов .html или .htm.\n"
                << "Нажмите любую клавишу для возврата...\n";

            input_data_reader(true, false, false);
            return "";
        }

        for (std::size_t i = 0; i < files.size(); ++i) {
            std::cout << i + 1 << ". " << files[i] << "\n";
        }

        std::cout << "Введите номер файла (Esc — отмена) > ";

        Input_data input = input_data_reader(true, true, false);

        if (input.Esc_controller) {
            return "";
        }

        if (!input.Enter_controller ||
            input.result.empty() ||
            !contains_only(input.result, 'ц')) {
            continue;
        }

        try {
            const unsigned long choice = std::stoul(input.result);

            if (choice >= 1 && choice <= files.size()) {
                return files[choice - 1];
            }
        } catch (const std::exception&) {
            // При некорректном номере повторяем запрос.
        }
    }
}

void show_about_screen() {
    system("cls");

    std::cout
        << "=======================================================================================================================\n";
    std::cout << "                                         ИНФОРМАЦИЯ О ПРОГРАММЕ\n";
    std::cout
        << "Программа вручную разбирает теги, атрибуты, комментарии и текст.\n";
    std::cout
        << "Вложенность парных тегов проверяется с помощью стека.\n";
    std::cout
        << "Это проверка поддерживаемого подмножества HTML, а не полная валидация HTML5.\n";
    std::cout << "Нажмите любую клавишу для возврата...\n";

    input_data_reader(true, false, false);
}

void show_completion_screen(
    const std::string& filename,
    std::size_t errors) {
    std::cout << "\nФайл: " << filename
              << "\nКоличество диагностик: " << errors << "\n";
    std::cout
        << "Нажмите любую клавишу для возврата в главное меню...\n";

    input_data_reader(true, false, false);
}
