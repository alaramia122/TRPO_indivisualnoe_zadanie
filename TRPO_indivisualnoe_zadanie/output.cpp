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

// Добавляет Unicode-кодовую точку в UTF-8.
void append_utf8(std::string& output, unsigned long codepoint) {
    if (codepoint <= 0x7F) {
        output += static_cast<char>(codepoint);
    } else if (codepoint <= 0x7FF) {
        output += static_cast<char>(0xC0 | (codepoint >> 6));
        output += static_cast<char>(0x80 | (codepoint & 0x3F));
    } else if (codepoint <= 0xFFFF &&
               !(codepoint >= 0xD800 && codepoint <= 0xDFFF)) {
        output += static_cast<char>(0xE0 | (codepoint >> 12));
        output += static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F));
        output += static_cast<char>(0x80 | (codepoint & 0x3F));
    } else if (codepoint <= 0x10FFFF) {
        output += static_cast<char>(0xF0 | (codepoint >> 18));
        output += static_cast<char>(0x80 | ((codepoint >> 12) & 0x3F));
        output += static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F));
        output += static_cast<char>(0x80 | (codepoint & 0x3F));
    }
}

// Декодирует распространённые HTML-сущности и числовые ссылки.
std::string decode_html_entities(const std::string& value) {
    std::string result;
    for (std::size_t i = 0; i < value.size();) {
        if (value[i] != '&') {
            result += value[i++];
            continue;
        }
        const std::size_t semicolon = value.find(';', i + 1);
        if (semicolon == std::string::npos || semicolon - i > 12) {
            result += value[i++];
            continue;
        }
        const std::string entity = value.substr(i + 1, semicolon - i - 1);
        if (entity == "amp") result += '&';
        else if (entity == "lt") result += '<';
        else if (entity == "gt") result += '>';
        else if (entity == "quot") result += '"';
        else if (entity == "apos") result += '\'';
        else if (entity == "nbsp") result += ' ';
        else if (entity == "ndash") result += "\xE2\x80\x93";
        else if (entity == "mdash") result += "\xE2\x80\x94";
        else if (entity == "hellip") result += "\xE2\x80\xA6";
        else if (entity == "copy") result += "\xC2\xA9";
        else if (entity == "reg") result += "\xC2\xAE";
        else if (!entity.empty() && entity[0] == '#') {
            try {
                const bool hexadecimal = entity.size() > 2 &&
                    (entity[1] == 'x' || entity[1] == 'X');
                const std::string digits = entity.substr(hexadecimal ? 2 : 1);
                const unsigned long codepoint = std::stoul(
                    digits, nullptr, hexadecimal ? 16 : 10);
                if (codepoint == 0 || codepoint > 0x10FFFF ||
                    (codepoint >= 0xD800 && codepoint <= 0xDFFF)) {
                    result.append(value, i, semicolon - i + 1);
                } else {
                    append_utf8(result, codepoint);
                }
            } catch (...) {
                result.append(value, i, semicolon - i + 1);
            }
        } else {
            result.append(value, i, semicolon - i + 1);
        }
        i = semicolon + 1;
    }
    return result;
}

// Извлекает метаданные, текстовые элементы, ссылки, изображения и поля форм.
std::vector<std::pair<std::string, std::vector<std::string>>>
collect_document_information(const HtmlAnalysisResult& result) {
    std::string active_text_tag;
    std::string text_buffer;
    std::string active_link;
    std::string link_href;
    bool collecting_link = false;
    bool inside_raw_text = false;
    std::string raw_text_tag;

    std::string document_title;
    std::vector<std::string> headings;
    std::vector<std::string> links;
    std::vector<std::string> images;
    std::vector<std::string> metadata;
    std::vector<std::string> page_text;
    std::vector<std::string> form_fields;
    std::vector<std::string> canonical_urls;

    for (const HtmlToken& token : result.tokens) {
        if (token.type == HtmlTokenType::OpenTag ||
            token.type == HtmlTokenType::SelfClosingTag) {
            if (token.name == "script" || token.name == "style") {
                inside_raw_text = token.type == HtmlTokenType::OpenTag;
                raw_text_tag = inside_raw_text ? token.name : "";
            }

            if (token.name == "meta") {
                const std::string name = get_attribute_value(token, "name");
                const std::string property = get_attribute_value(token, "property");
                const std::string content = decode_html_entities(
                    get_attribute_value(token, "content"));
                const std::string charset = get_attribute_value(token, "charset");
                if (!charset.empty()) metadata.push_back("Кодировка: " + charset);
                if (!name.empty() && !content.empty() &&
                    (name == "description" || name == "keywords" ||
                     name == "author" || name == "application-name")) {
                    metadata.push_back(name + ": " + utf8_to_console(content));
                }
                if (!property.empty() && !content.empty() &&
                    (property == "og:title" || property == "og:description" ||
                     property == "og:type" || property == "og:url")) {
                    metadata.push_back(property + ": " + utf8_to_console(content));
                }
            }

            if (token.name == "link") {
                const std::string rel = get_attribute_value(token, "rel");
                const std::string href = get_attribute_value(token, "href");
                if (rel == "canonical" && !href.empty()) {
                    canonical_urls.push_back(utf8_to_console(href));
                }
            }

            if (token.name == "img") {
                const std::string alt = normalize_text(decode_html_entities(
                    get_attribute_value(token, "alt")));
                std::string src = get_attribute_value(token, "src");
                if (src.empty()) {
                    src = get_attribute_value(token, "srcset");
                }

                // Скрытый счётчик аналитики не относится к содержимому страницы.
                const bool tracking_pixel =
                    src.find("mc.yandex.ru/watch") != std::string::npos;
                if ((!alt.empty() || !src.empty()) && !tracking_pixel) {
                    std::string image_info = alt.empty()
                        ? "изображение"
                        : utf8_to_console(alt);
                    if (!src.empty()) {
                        image_info += " [" + utf8_to_console(src) + "]";
                    }
                    images.push_back(image_info);
                }
            }

            if (token.name == "a" && token.type == HtmlTokenType::OpenTag) {
                active_link.clear();
                link_href = get_attribute_value(token, "href");
                collecting_link = true;
            }

            if (token.name == "input" || token.name == "textarea" ||
                token.name == "select" || token.name == "button") {
                std::string field = token.name;
                const std::string type = get_attribute_value(token, "type");
                const std::string name = get_attribute_value(token, "name");
                const std::string placeholder = decode_html_entities(
                    get_attribute_value(token, "placeholder"));
                const std::string value = decode_html_entities(
                    get_attribute_value(token, "value"));
                if (!type.empty()) field += " type=" + type;
                if (!name.empty()) field += " name=" + utf8_to_console(name);
                if (!placeholder.empty()) {
                    field += " placeholder=" + utf8_to_console(placeholder);
                }
                if (!value.empty()) field += " value=" + utf8_to_console(value);
                if (field != token.name) form_fields.push_back(field);
            }

            if (active_text_tag.empty() &&
                (token.name == "title" || is_heading_tag(token.name)) &&
                token.type == HtmlTokenType::OpenTag) {
                active_text_tag = token.name;
                text_buffer.clear();
            }
        }

        if (token.type == HtmlTokenType::Text) {
            const std::string decoded = decode_html_entities(token.raw_text);
            if (!active_text_tag.empty()) text_buffer += decoded;
            if (collecting_link) active_link += decoded;
            if (!inside_raw_text) {
                const std::string text = normalize_text(decoded);
                if (text.size() >= 2) page_text.push_back(utf8_to_console(text));
            }
        }

        if (token.type == HtmlTokenType::CloseTag) {
            if (token.name == raw_text_tag) {
                inside_raw_text = false;
                raw_text_tag.clear();
            }

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
    sections.push_back({"Метаданные", metadata});
    sections.push_back({"Заголовки разделов", headings});
    sections.push_back({"Текст страницы", page_text});
    sections.push_back({"Ссылки", links});
    sections.push_back({"Изображения", images});
    sections.push_back({"Элементы форм", form_fields});
    sections.push_back({"Канонический URL", canonical_urls});

    for (auto& section : sections) {
        if (section.second.empty()) section.second.push_back("Список пуст.");
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
