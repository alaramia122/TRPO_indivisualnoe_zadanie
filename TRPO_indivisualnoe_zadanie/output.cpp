#include "Header.h"


// Возвращает значение атрибута с указанным именем.
std::string get_attribute_value(
    const HtmlToken& token,
    const std::string& name) {
    for (const HtmlAttribute& attribute : token.attributes) {    // Сравниваем имена атрибутов без учёта регистра, сохраняя исходное значение атрибута.
        std::string attribute_name = attribute.name;
        std::transform(    // Приводим имя атрибута к единому регистру для сравнения.
            attribute_name.begin(),
            attribute_name.end(),
            attribute_name.begin(),
            [](unsigned char ch) {
                return static_cast<char>(std::tolower(ch));
            });

        if (attribute_name == name) {
            return attribute.value;
        }
    }

    return "";    // Возвращаем пустое значение, если подходящий атрибут не найден.
}

// Убирает пробелы по краям и сводит последовательности пробелов к одному.
std::string normalize_text(const std::string& value) {
    // Нормализация нужна для компактного вывода текста, извлечённого из разметки.
        // Создаём строку, в которой будет накапливаться результат обработки.
    std::string result;
    bool pending_space = false;    // Отмечаем наличие пробела, который потребуется вставить между словами.

    for (std::size_t i = 0; i < value.size();) {
        if (i + 1 < value.size() &&    // Неразрывный пробел в UTF-8 занимает два байта.
            static_cast<unsigned char>(value[i]) == 0xC2 &&
            static_cast<unsigned char>(value[i + 1]) == 0xA0) {
            pending_space = !result.empty();
            i += 2;
            continue;
        }

        const unsigned char ch =
            static_cast<unsigned char>(value[i++]);
        const bool is_ascii_space =
            ch == ' ' || ch == '\t' || ch == '\n' ||
            ch == '\r' || ch == '\f' || ch == '\v';

        if (is_ascii_space) {
            pending_space = !result.empty();
            continue;
        }

        if (pending_space) {
            result += ' ';
            pending_space = false;
        }

        result += static_cast<char>(ch);
    }

    return result;    // Возвращаем сформированный результат обработки.
}

// Проверяет, относится ли имя тега к заголовкам h1–h6.
bool is_heading_tag(const std::string& name) {
    return name.size() == 2 &&    // Учитываем только стандартные имена заголовков с одной цифрой от 1 до 6.
        name[0] == 'h' &&
        name[1] >= '1' &&
        name[1] <= '6';
}

// Храним извлечённые сведения отдельно от логики обхода HTML-токенов.
struct DocumentInfo {
    std::string title;
    std::vector<std::string> metadata, headings, page_text;
};

// Сохраняем контекст открытых элементов между соседними токенами.
struct ExtractionState {
    std::string text_tag, text_buffer, raw_tag;
    bool inside_raw_text = false;
};

// Извлекаем поддерживаемые meta-поля и свойства Open Graph.
void extract_metadata(const HtmlToken& token, DocumentInfo& info) {
    const std::string name = get_attribute_value(token, "name");
    const std::string property = get_attribute_value(token, "property");
    const std::string content = get_attribute_value(token, "content");
    const std::string charset = get_attribute_value(token, "charset");
    if (!charset.empty()) info.metadata.push_back("Кодировка: " + charset);    // Кодировку сохраняем отдельно: для неё не требуется атрибут content.
    if (!content.empty() &&
        (name == "description" || name == "keywords" || name == "author" ||
         name == "application-name")) info.metadata.push_back(name + ": " + content);
    if (!content.empty() &&
        (property == "og:title" || property == "og:description" ||
         property == "og:type")) {
        info.metadata.push_back(property + ": " + content);
    }
}

// Обрабатываем данные открывающего или самозакрывающегося тега.
void process_open_tag(const HtmlToken& token, DocumentInfo& info, ExtractionState& state) {
    if (token.name == "script" || token.name == "style") {    // Содержимое script/style не должно попадать в обычный текст страницы.
        state.inside_raw_text = token.type == HtmlTokenType::OpenTag;
        state.raw_tag = state.inside_raw_text ? token.name : "";
    }
    if (token.name == "meta") extract_metadata(token, info);
    if (state.text_tag.empty() && token.type == HtmlTokenType::OpenTag &&    // Для title и заголовков собираем текст из нескольких токенов.
        (token.name == "title" || is_heading_tag(token.name))) {
        state.text_tag = token.name;
        state.text_buffer.clear();
    }
}

// Добавляем текстовый токен в активные элементы и в текст страницы.
void process_text_token(const HtmlToken& token, DocumentInfo& info, ExtractionState& state) {
    const std::string& text = token.raw_text;
    if (!state.text_tag.empty()) state.text_buffer += text;
    if (!state.inside_raw_text) {    // Не извлекаем код скриптов и стилей как пользовательский текст.
        const std::string normalized = normalize_text(text);
        if (normalized.size() >= 2) info.page_text.push_back(normalized);
    }
}

// Завершаем накопление данных при закрытии соответствующего элемента.
void process_close_tag(const HtmlToken& token, DocumentInfo& info, ExtractionState& state) {
    if (token.name == state.raw_tag) {
        state.inside_raw_text = false;
        state.raw_tag.clear();
    }
    if (!state.text_tag.empty() && token.name == state.text_tag) {
        const std::string text = normalize_text(state.text_buffer);
        if (state.text_tag == "title" && info.title.empty()) info.title = text;
        else if (state.text_tag != "title" && !text.empty()) {
            info.headings.push_back(state.text_tag + ": " + text);
        }
        state.text_tag.clear();
        state.text_buffer.clear();
    }
}

std::vector<std::pair<std::string, std::vector<std::string>>>    // Формируем разделы в едином порядке для интерфейса и отчёта.
build_information_sections(const DocumentInfo& info) {
    std::vector<std::pair<std::string, std::vector<std::string>>> sections = {
        {"Название документа", {info.title.empty() ? "(пусто)" : info.title}},
        {"Метаданные", info.metadata}, {"Заголовки разделов", info.headings},
        {"Текст страницы", info.page_text}
    };
    for (auto& section : sections) {    // Пустые разделы тоже должны содержать значение для навигации по результатам.
        if (section.second.empty()) section.second.push_back("Список пуст.");
    }
    return sections;
}

std::vector<std::pair<std::string, std::vector<std::string>>>    // Последовательно обрабатываем токены и собираем сведения обо всём документе.
collect_document_information(const HtmlAnalysisResult& result) {
    DocumentInfo info;
    ExtractionState state;
    for (const HtmlToken& token : result.tokens) {
        if (token.type == HtmlTokenType::OpenTag || token.type == HtmlTokenType::SelfClosingTag) {
            process_open_tag(token, info, state);
        } else if (token.type == HtmlTokenType::Text) {
            process_text_token(token, info, state);
        } else if (token.type == HtmlTokenType::CloseTag) {
            process_close_tag(token, info, state);
        }
    }
    return build_information_sections(info);
}

// Выводит статистику анализа HTML.
void print_analysis_statistics(const HtmlAnalysisResult& result) {
    std::cout << std::string(100, '=') << "\n";    // Показываем заголовок и накопленные во время анализа счётчики.
    std::cout << "                                          РЕЗУЛЬТАТ АНАЛИЗА HTML\n";
    std::cout << "Открывающие теги:       " << result.opening_tags << "\n";
    std::cout << "Закрывающие теги:       " << result.closing_tags << "\n";
    std::cout << "Самозакрывающиеся теги: " << result.self_closing_tags << "\n";
    std::cout << "Атрибуты:               " << result.attributes << "\n";
    std::cout << "Комментарии:            " << result.comments << "\n";
    std::cout << "Текстовые фрагменты:    " << result.text_segments << "\n";
    std::cout << "Диагностики:            " << result.diagnostics.size() << "\n";
}

// Показывает выбранный раздел и значение с поддержкой клавиш-стрелок.
void show_document_information(
    const std::vector<std::pair<std::string, std::vector<std::string>>>& sections,
    std::size_t section_index,
    std::size_t value_index) {
    const auto& section = sections[section_index];    // Индексы передаются из навигационного цикла show_analysis_result().
    const auto& values = section.second;
    std::cout << "\nИНФОРМАЦИЯ ИЗ ДОКУМЕНТА\n";
    std::cout << "Раздел " << section_index + 1 << " из "
              << sections.size() << ": " << section.first << "\n";
    std::cout << "Значение " << value_index + 1 << " из "
              << values.size() << "\n";
    std::cout << "\n  " << values[value_index] << "\n";
    std::cout << "\nСтрелки вверх/вниз — выбрать раздел.\n";
    std::cout << "Стрелки влево/вправо — переключить значение.\n";
    std::cout << "Enter или Esc — продолжить.\n";
}

// Выводит сообщения валидатора в порядке их обнаружения.
void print_diagnostics(const HtmlAnalysisResult& result) {
    if (result.diagnostics.empty()) {    // При отсутствии диагностик сообщаем об успешной проверке.
        std::cout
            << "Проверка поддерживаемого подмножества HTML завершена успешно.\n";
        return;
    }

    for (const HtmlDiagnostic& diagnostic : result.diagnostics) {
        std::cout << "[" << diagnostic.category << "] строка "
                  << diagnostic.line << ": "
                  << diagnostic.message << "\n";
    }
}

// Сохраняет статистику, извлечённые сведения и диагностики в UTF-8-файл.
bool save_analysis_report(
    const HtmlAnalysisResult& result,
    const std::string& source_filename,
    std::string& saved_path,
    std::string& error_message) {
    try {
        const fs::path output_directory("AnalysisResults");    // Создаём каталог для отчётов, если он ещё отсутствует.
        fs::create_directories(output_directory);

        const std::string base_name =
            fs::u8path(source_filename).stem().u8string() + "_analysis";
        fs::path report_path = output_directory / fs::u8path(base_name + ".txt");    // Формируем путь к новому отчёту на основе имени исходного файла.

        std::size_t suffix = 2;    // Не перезаписываем ранее сохранённый отчёт с таким же именем.
        while (fs::exists(report_path)) {
            report_path = output_directory / fs::u8path(base_name + "_" + std::to_string(suffix) + ".txt");
            ++suffix;
        }

        std::ofstream report(report_path, std::ios::binary);    // Открываем файл в бинарном режиме, чтобы записать UTF-8 без преобразования байтов.
        if (!report.is_open()) {
            error_message = "Не удалось открыть файл для записи.";
            return false;
        }

        report.write("\xEF\xBB\xBF", 3);    // Добавляем BOM, чтобы текстовые редакторы могли определить кодировку UTF-8.
        report << "РЕЗУЛЬТАТ АНАЛИЗА HTML" << "\n";    // Записываем заголовок отчёта.
        report << std::string(60, '=') << "\n\n";
        report << "Исходный файл: "    // Указываем путь к проанализированному файлу.
               << source_filename << "\n\n";
        report << "СТАТИСТИКА" << "\n";    // Сохраняем счётчики, полученные при разборе и проверке документа.
        report << "Открывающие теги: "
               << result.opening_tags << "\n";
        report << "Закрывающие теги: "
               << result.closing_tags << "\n";
        report << "Самозакрывающиеся теги: "
               << result.self_closing_tags << "\n";
        report << "Атрибуты: "
               << result.attributes << "\n";
        report << "Комментарии: "
               << result.comments << "\n";
        report << "Текстовые фрагменты: "
               << result.text_segments << "\n";
        report << "Диагностики: "
               << result.diagnostics.size() << "\n";

        const auto sections = collect_document_information(result);    // Используем тот же набор разделов, который показывается в интерфейсе.
        for (const auto& section : sections) {    // Записываем извлечённые данные в том же порядке, что и в интерфейсе.
            report << "\n" << section.first << "\n";
            report << std::string(60, '-') << "\n";

            for (const std::string& value : section.second) {
                report << "- " << value << "\n";
            }
        }

        report << "\n" << "ДИАГНОСТИКИ" << "\n";    // В конце отчёта сохраняем все обнаруженные диагностики.
        report << std::string(60, '-') << "\n";
        if (result.diagnostics.empty()) {    // При отсутствии замечаний сохраняем сообщение об успешной проверке.
            report << 
                "Проверка поддерживаемого подмножества HTML завершена успешно."
                   << "\n";
        } else {
            for (const HtmlDiagnostic& diagnostic : result.diagnostics) {
                report << "[" << diagnostic.category << "] "
                       << "строка " << diagnostic.line
                       << ": " << diagnostic.message << "\n";
            }
        }

        report.flush();    // Проверяем состояние потока после сброса буфера на диск.
        if (!report.good()) {
            error_message = "При записи отчёта произошла ошибка.";
            return false;
        }

        saved_path = report_path.u8string();
        return true;
    } catch (const std::exception&) {
        error_message = "Не удалось сохранить отчёт из-за ошибки файловой системы.";
        return false;
    }
}

// Отображает статистику, извлечённые сведения и диагностики HTML.
void show_analysis_result(
    const HtmlAnalysisResult& result,
    const std::string& source_filename) {
    const auto sections = collect_document_information(result);    // Подготавливаем разделы один раз: они используются при каждой перерисовке экрана.
    std::size_t section_index = 0;    // Начинаем навигацию с первого раздела результата.
    std::size_t value_index = 0;    // Начинаем просмотр с первого значения выбранного раздела.
    bool finished = false;    // Цикл завершается после Enter или Esc.

    while (!finished) {
        system("cls");    // Очищаем консоль перед отображением нового экрана.
        print_analysis_statistics(result);    // Выводим сводные счётчики анализа перед подробными данными.
        show_document_information(sections, section_index, value_index);    // Показываем выбранный раздел и его текущее значение.
        std::cout << "\n";
        print_diagnostics(result);    // Показываем диагностики, найденные при проверке HTML.

        const int key = _getch();    // Считываем нажатие клавиши без подтверждения клавишей Enter.

        if (key == 0 || key == 224) {    // _getch возвращает специальный префикс перед кодом клавиши-стрелки в Windows-консоли.
            const int arrow = _getch();    // Считываем код стрелки после специального префикса клавиши.

            switch (arrow) {    // Стрелки переключают раздел либо значение внутри текущего раздела.
            case 72:
                section_index = section_index == 0
                    ? sections.size() - 1
                    : section_index - 1;
                value_index = 0;
                break;
            case 80:
                section_index = (section_index + 1) % sections.size();
                value_index = 0;
                break;
            case 75:
                if (value_index == 0) {
                    value_index = sections[section_index].second.size() - 1;
                } else {
                    --value_index;
                }
                break;
            case 77:
                value_index = (value_index + 1) %
                    sections[section_index].second.size();
                break;
            default:
                break;
            }
        } else if (key == 13 || key == 27) {    // Enter и Esc завершают просмотр результата и открывают предложение сохранить отчёт.
            finished = true;
        }
    }

    show_save_prompt(result, source_filename);    // Предлагаем пользователю сохранить сформированный результат.
}
