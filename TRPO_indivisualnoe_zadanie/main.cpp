#include "Header.h"

int main() {
    // Настраиваем консоль на русскую кодовую страницу Windows-1251.
    SetConsoleCP(1251);
    SetConsoleOutputCP(1251);

    if (setlocale(LC_ALL, "ru") == nullptr) {
        setlocale(LC_ALL, "Russian_Russia.1251");
    }

    system("color F0");
    show_start_window();

    while (true) {
        const int choice = show_main_menu();

        if (choice == 1) {
            const std::string filename = choose_source_file();

            if (filename.empty()) {
                continue;
            }

            const std::string path =
                (fs::path("SourceTexts") / filename).string();

            std::string source;
            std::string error_message;

            if (!read_text_file(path, source, error_message)) {
                system("cls");

                std::cout << error_message
                          << "\nНажмите любую клавишу для возврата...\n";

                input_data_reader(true, false, false);
                continue;
            }

            HtmlAnalysisResult result;
            parse_html(source, result);
            validate_html_structure(result);
            show_analysis_result(result);
        } else if (choice == 2) {
            show_about_screen();
        } else {
            break;
        }
    }

    return 0;
}
