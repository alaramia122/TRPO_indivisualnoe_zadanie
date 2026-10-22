#include "Header.h"

namespace {

// Преобразует текст HTML из UTF-8 в кодировку консольного интерфейса Windows-1251.
std::string utf8_to_console(const std::string& value) {
    if (value.empty()) {
        return value;
    }

    const int wide_size = MultiByteToWideChar(
        CP_UTF8,
        MB_ERR_INVALID_CHARS,
        value.data(),
        static_cast<int>(value.size()),
        nullptr,
        0);

    if (wide_size == 0) {
        return value;
    }

    std::wstring wide_text(static_cast<std::size_t>(wide_size), L'\0');
    MultiByteToWideChar(
        CP_UTF8,
        MB_ERR_INVALID_CHARS,
        value.data(),
        static_cast<int>(value.size()),
        wide_text.data(),
        wide_size);

    const int output_size = WideCharToMultiByte(
        1251,
        0,
        wide_text.data(),
        wide_size,
        nullptr,
        0,
        nullptr,
        nullptr);

    if (output_size == 0) {
        return value;
    }

    std::string converted(static_cast<std::size_t>(output_size), '\0');
    WideCharToMultiByte(
        1251,
        0,
        wide_text.data(),
        wide_size,
        converted.data(),
        output_size,
        nullptr,
        nullptr);

    return converted;
}

// Возвращает значение атрибута с указанным именем.
std::string get_attribute_value(
    const HtmlToken& token,
    const std::string& name) {
    for (const HtmlAttribute& attribute : token.attributes) {
        std::string attribute_name = attribute.name;
        std::transform(
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

    return "";
}

// Убирает пробелы по краям и сводит последовательности пробелов к одному.
std::string normalize_text(const std::string& value) {
    std::string result;
    bool pending_space = false;

    for (unsigned char ch : value) {
        if (std::isspace(ch)) {
            pending_space = !result.empty();
            continue;
        }

        if (pending_space) {
            result += ' ';
            pending_space = false;
        }

        result += static_cast<char>(ch);
    }

    return result;
}

// Проверяет, относится ли имя тега к заголовкам h1–h6.
bool is_heading_tag(const std::string& name) {
    return name.size() == 2 &&
        name[0] == 'h' &&
        name[1] >= '1' &&
        name[1] <= '6';
}

// Извлекает название документа, заголовки разделов, ссылки и изображения.
std::vector<std::pair<std::string, std::vector<std::string>>>
collect_document_information(const HtmlAnalysisResult& result) {
    std::string active_text_tag;
    std::string text_buffer;
    std::string active_link;
    std::string link_href;
    bool collecting_link = false;

    std::vector<std::string> headings;
    std::vector<std::string> links;
    std::vector<std::string> images;
    std::string document_title;

    for (const HtmlToken& token : result.tokens) {
        if (token.type == HtmlTokenType::OpenTag ||
            token.type == HtmlTokenType::SelfClosingTag) {
            if (token.name == "img") {
                const std::string alt = normalize_text(
                    get_attribute_value(token, "alt"));
                const std::string src = get_attribute_value(token, "src");

                if (!alt.empty() || !src.empty()) {
                    std::string image_info = alt.empty()
                        ? "изображение"
                        : utf8_to_console(alt);

                    if (!src.empty()) {
                        image_info += " [" + utf8_to_console(src) + "]";
                    }

                    images.push_back(image_info);
                }
            }

            if (token.name == "a" &&
                token.type == HtmlTokenType::OpenTag) {
                active_link.clear();
                link_href = get_attribute_value(token, "href");
                collecting_link = true;
            }

            if (active_text_tag.empty() &&
                (token.name == "title" || is_heading_tag(token.name)) &&
                token.type == HtmlTokenType::OpenTag) {
                active_text_tag = token.name;
                text_buffer.clear();
            }
        }

        if (token.type == HtmlTokenType::Text) {
            if (!active_text_tag.empty()) {
                text_buffer += token.raw_text;
            }

            if (collecting_link) {
                active_link += token.raw_text;
            }
        }

        if (token.type == HtmlTokenType::CloseTag) {
            if (token.name == "a" && collecting_link) {
                const std::string label = normalize_text(active_link);
                std::string link_info = label.empty()
                    ? utf8_to_console(link_href)
                    : utf8_to_console(label);

                if (!link_href.empty() && !label.empty()) {
                    link_info += " -> " + utf8_to_console(link_href);
                }

                links.push_back(link_info);
                active_link.clear();
                link_href.clear();
                collecting_link = false;
            }

            if (!active_text_tag.empty() && token.name == active_text_tag) {
                const std::string extracted = normalize_text(text_buffer);

                if (active_text_tag == "title") {
                    if (document_title.empty()) {
                        document_title = utf8_to_console(extracted);
                    }
                } else if (!extracted.empty()) {
                    headings.push_back(
                        active_text_tag + ": " + utf8_to_console(extracted));
                }

                active_text_tag.clear();
                text_buffer.clear();
            }
        }
    }

    std::vector<std::pair<std::string, std::vector<std::string>>> sections;
    sections.push_back({
        "Название документа",
        {document_title.empty() ? "(пусто)" : document_title}
    });
    sections.push_back({"Заголовки разделов", headings});
    sections.push_back({"Ссылки", links});
    sections.push_back({"Изображения", images});

    for (auto& section : sections) {
        if (section.second.empty()) {
            section.second.push_back("Список пуст.");
        }
    }

    return sections;
}

// Выводит статистику анализа HTML.
void print_analysis_statistics(const HtmlAnalysisResult& result) {
    std::cout << std::string(100, '=') << "\n";
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
    const auto& section = sections[section_index];
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
    if (result.diagnostics.empty()) {
        std::cout
            << "Проверка поддерживаемого подмножества HTML завершена успешно.\n";
        return;
    }

    for (const HtmlDiagnostic& diagnostic : result.diagnostics) {
        std::cout << "[" << diagnostic.category << "] строка "
                  << diagnostic.line << ", столбец "
                  << diagnostic.column << ": "
                  << utf8_to_console(diagnostic.message) << "\n";
    }
}

}

// Отображает статистику, извлечённые сведения и диагностики HTML.
void show_analysis_result(const HtmlAnalysisResult& result) {
    const auto sections = collect_document_information(result);
    std::size_t section_index = 0;
    std::size_t value_index = 0;
    bool finished = false;

    while (!finished) {
        system("cls");
        print_analysis_statistics(result);
        show_document_information(sections, section_index, value_index);
        std::cout << "\n";
        print_diagnostics(result);

        const int key = _getch();

        if (key == 0 || key == 224) {
            const int arrow = _getch();

            switch (arrow) {
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
        } else if (key == 13 || key == 27) {
            finished = true;
        }
    }

    std::cout << "\nНажмите любую клавишу для возврата...\n";
    input_data_reader(true, false, false);
}
