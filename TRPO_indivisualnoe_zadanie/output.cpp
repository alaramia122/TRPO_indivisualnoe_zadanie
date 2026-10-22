#include "Header.h"

void show_analysis_result(const HtmlAnalysisResult& result) {
    system("cls");

    std::cout
        << "=======================================================================================================================\n";
    std::cout
        << "                                          РЕЗУЛЬТАТ АНАЛИЗА HTML\n";

    // Выводим статистику по обработанному документу.
    std::cout << "Открывающие теги:       "
              << result.opening_tags << "\n";
    std::cout << "Закрывающие теги:       "
              << result.closing_tags << "\n";
    std::cout << "Самозакрывающиеся теги: "
              << result.self_closing_tags << "\n";
    std::cout << "Атрибуты:               "
              << result.attributes << "\n";
    std::cout << "Комментарии:            "
              << result.comments << "\n";
    std::cout << "Текстовые фрагменты:    "
              << result.text_segments << "\n";
    std::cout << "Диагностики:            "
              << result.diagnostics.size() << "\n";

    if (result.diagnostics.empty()) {
        std::cout
            << "В пределах поддерживаемого подмножества HTML ошибки не обнаружены.\n";
    } else {
        for (const HtmlDiagnostic& diagnostic : result.diagnostics) {
            std::cout << "[" << diagnostic.category << "] строка "
                      << diagnostic.line << ", столбец "
                      << diagnostic.column << ": "
                      << diagnostic.message << "\n";
        }
    }

    std::cout << "\nНажмите любую клавишу для возврата...\n";
    input_data_reader(true, false, false);
}
