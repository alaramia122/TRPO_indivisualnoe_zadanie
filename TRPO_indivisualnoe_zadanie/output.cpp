#include "Header.h"

namespace {

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

// Собирает текст заголовка документа, заголовков разделов, ссылок и изображений.
void show_document_information(const HtmlAnalysisResult& result) {
    std::string active_text_tag;
    std::string text_buffer;
    std::string active_link;
    std::string link_href;
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
                        : alt;
                    if (!src.empty()) {
                        image_info += " [" + src + "]";
                    }
                    images.push_back(image_info);
                }
            }

            if (token.name == "a" &&
                token.type == HtmlTokenType::OpenTag) {
                active_link.clear();
                link_href = get_attribute_value(token, "href");
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

            if (!link_href.empty() || !active_link.empty()) {
                active_link += token.raw_text;
            }
        }

        if (token.type == HtmlTokenType::CloseTag) {
            if (token.name == "a" && (!link_href.empty() || !active_link.empty())) {
                const std::string label = normalize_text(active_link);
                std::string link_info = label.empty() ? link_href : label;
                if (!link_href.empty() && !label.empty()) {
                    link_info += " -> " + link_href;
                }
                links.push_back(link_info);
                active_link.clear();
                link_href.clear();
            }

            if (!active_text_tag.empty() && token.name == active_text_tag) {
                const std::string extracted = normalize_text(text_buffer);
                if (active_text_tag == "title") {
                    if (document_title.empty()) {
                        document_title = extracted;
                    }
                } else if (!extracted.empty()) {
                    headings.push_back(active_text_tag + ": " + extracted);
                }

                active_text_tag.clear();
                text_buffer.clear();
            }
        }
    }

    std::cout << "\nИНФОРМАЦИЯ ИЗ ДОКУМЕНТА\n";
    std::cout << "Название: "
              << (document_title.empty() ? "(пусто)" : document_title)
              << "\n";

    const auto print_items = [](const std::string& label,
                                const std::vector<std::string>& items) {
        std::cout << label << " (" << items.size() << "):\n";
        if (items.empty()) {
            std::cout << "  Список пуст.\n";
            return;
        }

        const std::size_t limit = 5;
        for (std::size_t i = 0; i < items.size() && i < limit; ++i) {
            std::cout << "  " << i + 1 << ". " << items[i] << "\n";
        }
        if (items.size() > limit) {
            std::cout << "  Ещё элементов: " << items.size() - limit << "\n";
        }
    };

    print_items("Заголовки разделов", headings);
    print_items("Ссылки", links);
    print_items("Изображения", images);
}

}

// Отображаем статистику анализа, полезные сведения о документе и список диагностик.
void show_analysis_result(const HtmlAnalysisResult& result) {
    system("cls");

    std::cout
        << std::string(100, '=') << "\n";
    std::cout
        << "                                          РЕЗУЛЬТАТ АНАЛИЗА HTML\n";

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

    show_document_information(result);

    if (result.diagnostics.empty()) {
        std::cout
            << "Проверка поддерживаемого подмножества HTML завершена успешно.\n";
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
