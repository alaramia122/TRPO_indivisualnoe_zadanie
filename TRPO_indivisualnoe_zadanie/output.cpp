#include "Header.h"

namespace {

// Возвращает значение атрибута с указанным именем.
std::string get_attribute_value(
    const HtmlToken& token,
    const std::string& name) {
    // Сравниваем имена атрибутов без учёта регистра, сохраняя исходное значение атрибута.
    for (const HtmlAttribute& attribute : token.attributes) {
        std::string attribute_name = attribute.name;
        // Приводим имя атрибута к единому регистру для сравнения.
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

    // Возвращаем пустое значение, если подходящий атрибут не найден.
    return "";
}

// Убирает пробелы по краям и сводит последовательности пробелов к одному.
std::string normalize_text(const std::string& value) {
    // Нормализация нужна для компактного вывода текста, извлечённого из разметки.
        // Создаём строку, в которой будет накапливаться результат обработки.
    std::string result;
    // Отмечаем наличие пробела, который потребуется вставить между словами.
    bool pending_space = false;

    for (std::size_t i = 0; i < value.size();) {
        // Неразрывный пробел в UTF-8 занимает два байта.
        if (i + 1 < value.size() &&
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

    // Возвращаем сформированный результат обработки.
    return result;
}

// Проверяет, относится ли имя тега к заголовкам h1–h6.
bool is_heading_tag(const std::string& name) {
    // Учитываем только стандартные имена заголовков с одной цифрой от 1 до 6.
    return name.size() == 2 &&
        name[0] == 'h' &&
        name[1] >= '1' &&
        name[1] <= '6';
}

// Преобразуем кодовую точку Unicode в UTF-8 средствами Windows API.
void append_utf8(std::string& output, unsigned long codepoint) {
    wchar_t wide[2];
    int wide_size = 1;
    if (codepoint > 0xFFFF) {
        codepoint -= 0x10000;
        wide[0] = static_cast<wchar_t>(0xD800 + (codepoint >> 10));
        wide[1] = static_cast<wchar_t>(0xDC00 + (codepoint & 0x3FF));
        wide_size = 2;
    } else wide[0] = static_cast<wchar_t>(codepoint);
    char bytes[4];
    const int size = WideCharToMultiByte(CP_UTF8, 0, wide, wide_size, bytes, sizeof(bytes), nullptr, nullptr);
    if (size > 0) output.append(bytes, static_cast<std::size_t>(size));
}

// Декодирует распространённые HTML-сущности и числовые ссылки.
std::string decode_html_entities(const std::string& value) {
    // Обычные символы копируются напрямую, а конструкции с амперсандом проверяются как HTML-сущности.
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
        // Выделяем имя сущности между амперсандом и точкой с запятой.
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
        // Числовые ссылки могут задаваться десятичным или шестнадцатеричным кодом Unicode.
        else if (!entity.empty() && entity[0] == '#') {
            try {
                // Определяем основание системы счисления числовой ссылки.
                const bool hexadecimal = entity.size() > 2 &&
                    (entity[1] == 'x' || entity[1] == 'X');
                // Выделяем цифры числовой ссылки без её префикса.
                const std::string digits = entity.substr(hexadecimal ? 2 : 1);
                // Получаем числовой код символа из HTML-сущности.
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
    // Возвращаем сформированный результат обработки.
    return result;
}

// Храним извлечённые сведения отдельно от логики обхода HTML-токенов.
struct DocumentInfo {
    std::string title;
    std::vector<std::string> metadata, headings, page_text, links, images, forms, canonical_urls;
};

// Сохраняем контекст открытых элементов между соседними токенами.
struct ExtractionState {
    std::string text_tag, text_buffer, link_text, link_href, raw_tag;
    bool collecting_link = false;
    bool inside_raw_text = false;
};

// Извлекаем поддерживаемые meta-поля и свойства Open Graph.
void extract_metadata(const HtmlToken& token, DocumentInfo& info) {
    const std::string name = get_attribute_value(token, "name");
    const std::string property = get_attribute_value(token, "property");
    const std::string content = decode_html_entities(get_attribute_value(token, "content"));
    const std::string charset = get_attribute_value(token, "charset");
    // Кодировку сохраняем отдельно: для неё не требуется атрибут content.
    if (!charset.empty()) info.metadata.push_back("Кодировка: " + charset);
    if (!content.empty() &&
        (name == "description" || name == "keywords" || name == "author" ||
         name == "application-name")) info.metadata.push_back(name + ": " + content);
    if (!content.empty() &&
        (property == "og:title" || property == "og:description" ||
         property == "og:type" || property == "og:url")) {
        info.metadata.push_back(property + ": " + content);
    }
}

// Формируем описание изображения по alt и адресу ресурса.
void extract_image(const HtmlToken& token, DocumentInfo& info) {
    const std::string alt = normalize_text(decode_html_entities(get_attribute_value(token, "alt")));
    std::string src = get_attribute_value(token, "src");
    // Используем srcset только как запасной источник, если src отсутствует.
    if (src.empty()) src = get_attribute_value(token, "srcset");
    // Не включаем известный счётчик аналитики в список изображений страницы.
    if ((!alt.empty() || !src.empty()) && src.find("mc.yandex.ru/watch") == std::string::npos) {
        std::string description = alt.empty() ? "изображение" : alt;
        if (!src.empty()) description += " [" + src + "]";
        info.images.push_back(description);
    }
}

// Собираем значимые атрибуты элемента формы в одну строку.
void extract_form_field(const HtmlToken& token, DocumentInfo& info) {
    std::string field = token.name;
    const std::string type = get_attribute_value(token, "type");
    const std::string name = get_attribute_value(token, "name");
    const std::string placeholder = decode_html_entities(get_attribute_value(token, "placeholder"));
    const std::string value = decode_html_entities(get_attribute_value(token, "value"));
    if (!type.empty()) field += " type=" + type;
    if (!name.empty()) field += " name=" + name;
    if (!placeholder.empty()) field += " placeholder=" + placeholder;
    if (!value.empty()) field += " value=" + value;
    // Элемент без дополнительных атрибутов не несёт полезных сведений для отчёта.
    if (field != token.name) info.forms.push_back(field);
}

// Обрабатываем данные открывающего или самозакрывающегося тега.
void process_open_tag(const HtmlToken& token, DocumentInfo& info, ExtractionState& state) {
    // Содержимое script/style не должно попадать в обычный текст страницы.
    if (token.name == "script" || token.name == "style") {
        state.inside_raw_text = token.type == HtmlTokenType::OpenTag;
        state.raw_tag = state.inside_raw_text ? token.name : "";
    }
    if (token.name == "meta") extract_metadata(token, info);
    // Канонический адрес хранится отдельно от обычных ссылок.
    if (token.name == "link" && get_attribute_value(token, "rel") == "canonical") {
        const std::string href = get_attribute_value(token, "href");
        if (!href.empty()) info.canonical_urls.push_back(href);
    }
    if (token.name == "img") extract_image(token, info);
    if (token.name == "input" || token.name == "textarea" ||
        token.name == "select" || token.name == "button") extract_form_field(token, info);
    // Накапливаем текст ссылки до появления соответствующего закрывающего тега.
    if (token.name == "a" && token.type == HtmlTokenType::OpenTag) {
        state.link_text.clear();
        state.link_href = get_attribute_value(token, "href");
        state.collecting_link = true;
    }
    // Для title и заголовков собираем текст из нескольких токенов.
    if (state.text_tag.empty() && token.type == HtmlTokenType::OpenTag &&
        (token.name == "title" || is_heading_tag(token.name))) {
        state.text_tag = token.name;
        state.text_buffer.clear();
    }
}

// Добавляем текстовый токен в активные элементы и в текст страницы.
void process_text_token(const HtmlToken& token, DocumentInfo& info, ExtractionState& state) {
    const std::string text = decode_html_entities(token.raw_text);
    if (!state.text_tag.empty()) state.text_buffer += text;
    if (state.collecting_link) state.link_text += text;
    // Не извлекаем код скриптов и стилей как пользовательский текст.
    if (!state.inside_raw_text) {
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
    if (token.name == "a" && state.collecting_link) {
        const std::string label = normalize_text(state.link_text);
        std::string link = label.empty() ? state.link_href : label;
        if (!state.link_href.empty() && !label.empty()) link += " -> " + state.link_href;
        info.links.push_back(link);
        state.link_text.clear();
        state.link_href.clear();
        state.collecting_link = false;
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

// Формируем разделы в едином порядке для интерфейса и отчёта.
std::vector<std::pair<std::string, std::vector<std::string>>>
build_information_sections(const DocumentInfo& info) {
    std::vector<std::pair<std::string, std::vector<std::string>>> sections = {
        {"Название документа", {info.title.empty() ? "(пусто)" : info.title}},
        {"Метаданные", info.metadata}, {"Заголовки разделов", info.headings},
        {"Текст страницы", info.page_text}, {"Ссылки", info.links},
        {"Изображения", info.images}, {"Элементы форм", info.forms},
        {"Канонический URL", info.canonical_urls}
    };
    // Пустые разделы тоже должны содержать значение для навигации по результатам.
    for (auto& section : sections) {
        if (section.second.empty()) section.second.push_back("Список пуст.");
    }
    return sections;
}

// Последовательно обрабатываем токены и собираем сведения обо всём документе.
std::vector<std::pair<std::string, std::vector<std::string>>>
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
    // Показываем заголовок и накопленные во время анализа счётчики.
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
    // Индексы передаются из навигационного цикла show_analysis_result().
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
    // При отсутствии диагностик сообщаем об успешной проверке.
    if (result.diagnostics.empty()) {
        std::cout
            << "Проверка поддерживаемого подмножества HTML завершена успешно.\n";
        return;
    }

    for (const HtmlDiagnostic& diagnostic : result.diagnostics) {
        std::cout << "[" << diagnostic.category << "] строка "
                  << diagnostic.line << ", столбец "
                  << diagnostic.column << ": "
                  << diagnostic.message << "\n";
    }
}

}

// Сохраняет статистику, извлечённые сведения и диагностики в UTF-8-файл.
bool save_analysis_report(
    const HtmlAnalysisResult& result,
    const std::string& source_filename,
    std::string& saved_path,
    std::string& error_message) {
    try {
        // Создаём каталог для отчётов, если он ещё отсутствует.
        const fs::path output_directory("AnalysisResults");
        fs::create_directories(output_directory);

        const std::string base_name =
            fs::u8path(source_filename).stem().u8string() + "_analysis";
        // Формируем путь к новому отчёту на основе имени исходного файла.
        fs::path report_path = output_directory / fs::u8path(base_name + ".txt");

        // Не перезаписываем ранее сохранённый отчёт с таким же именем.
        std::size_t suffix = 2;
        while (fs::exists(report_path)) {
            report_path = output_directory / fs::u8path(base_name + "_" + std::to_string(suffix) + ".txt");
            ++suffix;
        }

        // Открываем файл в бинарном режиме, чтобы записать UTF-8 без преобразования байтов.
        std::ofstream report(report_path, std::ios::binary);
        if (!report.is_open()) {
            error_message = "Не удалось открыть файл для записи.";
            return false;
        }

        // Добавляем BOM, чтобы текстовые редакторы могли определить кодировку UTF-8.
        report.write("\xEF\xBB\xBF", 3);
        // Записываем заголовок отчёта.
        report << "РЕЗУЛЬТАТ АНАЛИЗА HTML" << "\n";
        report << std::string(60, '=') << "\n\n";
        // Указываем путь к проанализированному файлу.
        report << "Исходный файл: "
               << source_filename << "\n\n";
        // Сохраняем счётчики, полученные при разборе и проверке документа.
        report << "СТАТИСТИКА" << "\n";
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

        // Используем тот же набор разделов, который показывается в интерфейсе.
        const auto sections = collect_document_information(result);
        // Записываем извлечённые данные в том же порядке, что и в интерфейсе.
        for (const auto& section : sections) {
            report << "\n" << section.first << "\n";
            report << std::string(60, '-') << "\n";

            for (const std::string& value : section.second) {
                report << "- " << value << "\n";
            }
        }

        // В конце отчёта сохраняем все обнаруженные диагностики.
        report << "\n" << "ДИАГНОСТИКИ" << "\n";
        report << std::string(60, '-') << "\n";
        // При отсутствии замечаний сохраняем сообщение об успешной проверке.
        if (result.diagnostics.empty()) {
            report << 
                "Проверка поддерживаемого подмножества HTML завершена успешно."
                   << "\n";
        } else {
            for (const HtmlDiagnostic& diagnostic : result.diagnostics) {
                report << "[" << diagnostic.category << "] "
                       << "строка " << diagnostic.line
                       << ", столбец " << diagnostic.column
                       << ": " << diagnostic.message << "\n";
            }
        }

        // Проверяем состояние потока после сброса буфера на диск.
        report.flush();
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
    // Подготавливаем разделы один раз: они используются при каждой перерисовке экрана.
    const auto sections = collect_document_information(result);
    // Начинаем навигацию с первого раздела результата.
    std::size_t section_index = 0;
    // Начинаем просмотр с первого значения выбранного раздела.
    std::size_t value_index = 0;
    // Цикл завершается после Enter или Esc.
    bool finished = false;

    while (!finished) {
        // Очищаем консоль перед отображением нового экрана.
        system("cls");
        // Выводим сводные счётчики анализа перед подробными данными.
        print_analysis_statistics(result);
        // Показываем выбранный раздел и его текущее значение.
        show_document_information(sections, section_index, value_index);
        std::cout << "\n";
        // Показываем диагностики, найденные при проверке HTML.
        print_diagnostics(result);

        // Считываем нажатие клавиши без подтверждения клавишей Enter.
        const int key = _getch();

        // _getch возвращает специальный префикс перед кодом клавиши-стрелки в Windows-консоли.
        if (key == 0 || key == 224) {
            // Считываем код стрелки после специального префикса клавиши.
            const int arrow = _getch();

            // Стрелки переключают раздел либо значение внутри текущего раздела.
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
        // Enter и Esc завершают просмотр результата и открывают предложение сохранить отчёт.
        } else if (key == 13 || key == 27) {
            finished = true;
        }
    }

    // Предлагаем пользователю сохранить сформированный результат.
    show_save_prompt(result, source_filename);
}
