#include "Header.h"

namespace {

// Исправляет распространённый случай текста UTF-8, ошибочно прочитанного как Windows-1251.
std::string repair_mojibake_utf8(const std::string& value) {
    // Сначала ищем характерные сочетания символов, возникающие при неверной интерпретации UTF-8.
    // Эвристика запускается только при наличии маркера, чтобы не преобразовывать обычный корректный текст.
    const bool has_mojibake_marker =
        value.find("Р ") != std::string::npos ||
        value.find("Рµ") != std::string::npos ||
        value.find("Рґ") != std::string::npos ||
        value.find("Р°") != std::string::npos ||
        value.find("Рє") != std::string::npos ||
        value.find("РЅ") != std::string::npos ||
        value.find("Рџ") != std::string::npos ||
        value.find("РЇ") != std::string::npos ||
        value.find("РЈ") != std::string::npos ||
        value.find("Рќ") != std::string::npos ||
        value.find("РЎ") != std::string::npos ||
        value.find("Рљ") != std::string::npos ||
        value.find("Р¤") != std::string::npos ||
        value.find("Рђ") != std::string::npos ||
        value.find("Рё") != std::string::npos ||
        value.find("Рѕ") != std::string::npos ||
        value.find("Рї") != std::string::npos ||
        value.find("Р±") != std::string::npos ||
        value.find("Р»") != std::string::npos ||
        value.find("Р·") != std::string::npos ||
        value.find("Рј") != std::string::npos ||
        value.find("С‚") != std::string::npos ||
        value.find("СЂ") != std::string::npos ||
        value.find("СЏ") != std::string::npos ||
        value.find("Сѓ") != std::string::npos ||
        value.find("СЃ") != std::string::npos ||
        value.find("В ") != std::string::npos;

    if (!has_mojibake_marker) {
        // Возвращаем исходное значение, если безопасное преобразование выполнить не удалось.
        return value;
    }

    // Проверяем, можно ли декодировать исходные байты как корректную строку UTF-8.
    const int wide_size = MultiByteToWideChar(
        CP_UTF8,
        MB_ERR_INVALID_CHARS,
        value.data(),
        static_cast<int>(value.size()),
        nullptr,
        0);

    if (wide_size == 0) {
        // Возвращаем исходное значение, если безопасное преобразование выполнить не удалось.
        return value;
    }

    std::wstring wide_text(static_cast<std::size_t>(wide_size), L'\0');
    if (MultiByteToWideChar(
            CP_UTF8,
            MB_ERR_INVALID_CHARS,
            value.data(),
            static_cast<int>(value.size()),
            wide_text.data(),
            wide_size) == 0) {
        // Возвращаем исходное значение, если безопасное преобразование выполнить не удалось.
        return value;
    }

    // Получаем представление тех же символов в Windows-1251, чтобы восстановить исходные байты ошибочно прочитанного текста.
    const int legacy_size = WideCharToMultiByte(
        1251,
        0,
        wide_text.data(),
        wide_size,
        nullptr,
        0,
        nullptr,
        nullptr);

    if (legacy_size == 0) {
        // Возвращаем исходное значение, если безопасное преобразование выполнить не удалось.
        return value;
    }

    std::string legacy_bytes(static_cast<std::size_t>(legacy_size), '\0');
    if (WideCharToMultiByte(
            1251,
            0,
            wide_text.data(),
            wide_size,
            legacy_bytes.data(),
            legacy_size,
            nullptr,
            nullptr) == 0) {
        // Возвращаем исходное значение, если безопасное преобразование выполнить не удалось.
        return value;
    }

    // Повторно декодируем полученные байты как UTF-8: на этом шаге обычно восстанавливается исходная кириллица.
    const int repaired_size = MultiByteToWideChar(
        CP_UTF8,
        MB_ERR_INVALID_CHARS,
        legacy_bytes.data(),
        legacy_size,
        nullptr,
        0);

    if (repaired_size == 0) {
        // Возвращаем исходное значение, если безопасное преобразование выполнить не удалось.
        return value;
    }

    // Создаём промежуточное Unicode-представление восстановленного текста.
    std::wstring repaired_wide(
        static_cast<std::size_t>(repaired_size), L'\0');
    if (MultiByteToWideChar(
            CP_UTF8,
            MB_ERR_INVALID_CHARS,
            legacy_bytes.data(),
            legacy_size,
            repaired_wide.data(),
            repaired_size) == 0) {
        // Возвращаем исходное значение, если безопасное преобразование выполнить не удалось.
        return value;
    }

    const int repaired_utf8_size = WideCharToMultiByte(
        CP_UTF8,
        0,
        repaired_wide.data(),
        repaired_size,
        nullptr,
        0,
        nullptr,
        nullptr);

    if (repaired_utf8_size == 0) {
        // Возвращаем исходное значение, если безопасное преобразование выполнить не удалось.
        return value;
    }

    // Выделяем буфер для восстановленной строки.
    std::string repaired(
        static_cast<std::size_t>(repaired_utf8_size), '\0');
    if (WideCharToMultiByte(
            CP_UTF8,
            0,
            repaired_wide.data(),
            repaired_size,
            repaired.data(),
            repaired_utf8_size,
            nullptr,
            nullptr) == 0) {
        // Возвращаем исходное значение, если безопасное преобразование выполнить не удалось.
        return value;
    }

    // Возвращаем строку после попытки восстановления кодировки.
    return repaired;
}

// Преобразует текст HTML из UTF-8 в кодировку консольного интерфейса Windows-1251.
std::string utf8_to_console(const std::string& value) {
    // Перед преобразованием исправляем известные случаи повреждённого текста и проверяем пустую строку.
    const std::string repaired = repair_mojibake_utf8(value);
    if (repaired.empty()) {
        // Возвращаем строку после попытки восстановления кодировки.
        return repaired;
    }

    // Промежуточная широкая строка позволяет Windows выполнить преобразование между кодировками.
    const int wide_size = MultiByteToWideChar(
        CP_UTF8,
        MB_ERR_INVALID_CHARS,
        repaired.data(),
        static_cast<int>(repaired.size()),
        nullptr,
        0);

    if (wide_size == 0) {
        // Возвращаем строку после попытки восстановления кодировки.
        return repaired;
    }

    std::wstring wide_text(static_cast<std::size_t>(wide_size), L'\0');
    if (MultiByteToWideChar(
            CP_UTF8,
            MB_ERR_INVALID_CHARS,
            repaired.data(),
            static_cast<int>(repaired.size()),
            wide_text.data(),
            wide_size) == 0) {
        // Возвращаем строку после попытки восстановления кодировки.
        return repaired;
    }

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
        // Возвращаем строку после попытки восстановления кодировки.
        return repaired;
    }

    std::string converted(static_cast<std::size_t>(output_size), '\0');
    if (WideCharToMultiByte(
            1251,
            0,
            wide_text.data(),
            wide_size,
            converted.data(),
            output_size,
            nullptr,
            nullptr) == 0) {
        // Возвращаем строку после попытки восстановления кодировки.
        return repaired;
    }

    // Возвращаем результат преобразования в целевую кодировку.
    return converted;
}

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
    // Исправляем повреждённую кодировку до обработки пробельных символов.
    const std::string repaired = repair_mojibake_utf8(value);
    // Создаём строку, в которой будет накапливаться результат обработки.
    std::string result;
    // Отмечаем наличие пробела, который потребуется вставить между словами.
    bool pending_space = false;

    for (std::size_t i = 0; i < repaired.size();) {
        // Неразрывный пробел в UTF-8 занимает два байта.
        if (i + 1 < repaired.size() &&
            static_cast<unsigned char>(repaired[i]) == 0xC2 &&
            static_cast<unsigned char>(repaired[i + 1]) == 0xA0) {
            pending_space = !result.empty();
            i += 2;
            continue;
        }

        const unsigned char ch =
            static_cast<unsigned char>(repaired[i++]);
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

// Добавляет Unicode-кодовую точку в UTF-8.
void append_utf8(std::string& output, unsigned long codepoint) {
    // Кодируем одну кодовую точку в UTF-8 в зависимости от её диапазона.
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

// Извлекает метаданные, текстовые элементы, ссылки, изображения и поля форм.
std::vector<std::pair<std::string, std::vector<std::string>>>
collect_document_information(const HtmlAnalysisResult& result) {
    // Для каждого раздела формируется отдельный список строк, пригодный и для интерфейса, и для отчёта.
    // Эти переменные накапливают текст текущего заголовка, названия документа или ссылки до закрывающего тега.
    std::string active_text_tag;
    // Накапливаем текст открытого заголовка до его закрытия.
    std::string text_buffer;
    // Накапливаем видимую подпись текущей ссылки.
    std::string active_link;
    // Сохраняем адрес текущей ссылки до завершения сбора её текста.
    std::string link_href;
    // Отслеживаем, идёт ли сейчас сбор текста ссылки.
    bool collecting_link = false;
    // Отслеживаем, находится ли обработка внутри script/style.
    bool inside_raw_text = false;
    // Запоминаем имя открытой области script/style для определения её конца.
    std::string raw_text_tag;

    // Результаты извлечения группируются по типам, чтобы отображать их в отдельных разделах.
    std::string document_title;
    // Храним найденные заголовки страницы.
    std::vector<std::string> headings;
    // Храним извлечённые ссылки и их подписи.
    std::vector<std::string> links;
    // Храним описания найденных изображений.
    std::vector<std::string> images;
    // Храним выбранные метаданные страницы.
    std::vector<std::string> metadata;
    // Храним текстовые фрагменты, извлечённые из документа.
    std::vector<std::string> page_text;
    // Храним описания элементов управления формы.
    std::vector<std::string> form_fields;
    // Храним найденные канонические URL.
    std::vector<std::string> canonical_urls;

    // Обрабатываем токены в исходном порядке, чтобы сохранять последовательность элементов страницы.
    for (const HtmlToken& token : result.tokens) {
        if (token.type == HtmlTokenType::OpenTag ||
            token.type == HtmlTokenType::SelfClosingTag) {
            if (token.name == "script" || token.name == "style") {
                inside_raw_text = token.type == HtmlTokenType::OpenTag;
                raw_text_tag = inside_raw_text ? token.name : "";
            }

            // Из meta извлекаются только выбранные поля, полезные для краткого описания страницы.
            if (token.name == "meta") {
                // Получаем имя атрибута или метаданных для дальнейшей проверки.
                const std::string name = get_attribute_value(token, "name");
                // Получаем свойство Open Graph для фильтрации метаданных.
                const std::string property = get_attribute_value(token, "property");
                // Считываем содержимое метаданных из атрибутов HTML.
                const std::string content = decode_html_entities(
                    get_attribute_value(token, "content"));
                // Получаем объявленную кодировку документа.
                const std::string charset = get_attribute_value(token, "charset");
                if (!charset.empty()) metadata.push_back("Кодировка: " + charset);
                if (!name.empty() && !content.empty() &&
                    (name == "description" || name == "keywords" ||
                     name == "author" || name == "application-name")) {
                    // Добавляем найденное значение метаданных в соответствующий список.
                    metadata.push_back(name + ": " + utf8_to_console(content));
                }
                if (!property.empty() && !content.empty() &&
                    (property == "og:title" || property == "og:description" ||
                     property == "og:type" || property == "og:url")) {
                    // Добавляем найденное значение метаданных в соответствующий список.
                    metadata.push_back(property + ": " + utf8_to_console(content));
                }
            }

            // Канонический адрес определяется по атрибуту rel="canonical".
            if (token.name == "link") {
                // Получаем назначение элемента link.
                const std::string rel = get_attribute_value(token, "rel");
                // Считываем адрес ссылки из атрибута href.
                const std::string href = get_attribute_value(token, "href");
                if (rel == "canonical" && !href.empty()) {
                    // Добавляем канонический адрес страницы в итоговый список.
                    canonical_urls.push_back(utf8_to_console(href));
                }
            }

            // Для изображения сохраняем альтернативное описание и адрес ресурса, если они доступны.
            if (token.name == "img") {
                // Получаем нормализованное альтернативное описание изображения.
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
                    // Формируем человекочитаемое описание изображения.
                    std::string image_info = alt.empty()
                        ? "изображение"
                        : utf8_to_console(alt);
                    if (!src.empty()) {
                        image_info += " [" + utf8_to_console(src) + "]";
                    }
                    // Добавляем описание изображения в список результатов.
                    images.push_back(image_info);
                }
            }

            // При открытии ссылки запоминаем адрес и начинаем собирать её видимый текст.
            if (token.name == "a" && token.type == HtmlTokenType::OpenTag) {
                active_link.clear();
                link_href = get_attribute_value(token, "href");
                collecting_link = true;
            }

            // Элементы управления добавляются в отчёт, если у них есть хотя бы один полезный атрибут.
            if (token.name == "input" || token.name == "textarea" ||
                token.name == "select" || token.name == "button") {
                std::string field = token.name;
                // Получаем тип элемента управления формы.
                const std::string type = get_attribute_value(token, "type");
                // Получаем имя атрибута или метаданных для дальнейшей проверки.
                const std::string name = get_attribute_value(token, "name");
                // Получаем текст подсказки элемента формы.
                const std::string placeholder = decode_html_entities(
                    get_attribute_value(token, "placeholder"));
                // Получаем значение атрибута или выбранного элемента.
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

        // Текстовый токен может одновременно относиться к заголовку, подписи ссылки и общему тексту страницы.
        if (token.type == HtmlTokenType::Text) {
            // Получаем декодированное значение для последующей обработки.
            const std::string decoded = decode_html_entities(token.raw_text);
            if (!active_text_tag.empty()) text_buffer += decoded;
            if (collecting_link) active_link += decoded;
            if (!inside_raw_text) {
                // Подготавливаем нормализованный текстовый фрагмент.
                const std::string text = normalize_text(decoded);
                if (text.size() >= 2) page_text.push_back(utf8_to_console(text));
            }
        }

        // Закрывающие теги завершают накопление текста для соответствующих элементов.
        if (token.type == HtmlTokenType::CloseTag) {
            if (token.name == raw_text_tag) {
                inside_raw_text = false;
                raw_text_tag.clear();
            }

            if (token.name == "a" && collecting_link) {
                // Подготавливаем подпись ссылки для отображения.
                const std::string label = normalize_text(active_link);
                // Формируем отображаемое описание ссылки.
                std::string link_info = label.empty()
                    ? utf8_to_console(link_href)
                    : utf8_to_console(label);
                if (!link_href.empty() && !label.empty()) {
                    link_info += " -> " + utf8_to_console(link_href);
                }
                // Добавляем описание ссылки в список результатов.
                links.push_back(link_info);
                active_link.clear();
                link_href.clear();
                collecting_link = false;
            }

            if (!active_text_tag.empty() && token.name == active_text_tag) {
                // Подготавливаем завершённый текстовый элемент для сохранения.
                const std::string extracted = normalize_text(text_buffer);
                if (active_text_tag == "title") {
                    if (document_title.empty()) {
                        document_title = utf8_to_console(extracted);
                    }
                } else if (!extracted.empty()) {
                    // Добавляем извлечённый заголовок в список заголовков документа.
                    headings.push_back(
                        active_text_tag + ": " + utf8_to_console(extracted));
                }
                active_text_tag.clear();
                text_buffer.clear();
            }
        }
    }

    // Фиксируем единый порядок разделов, используемый при навигации и формировании текстового отчёта.
    std::vector<std::pair<std::string, std::vector<std::string>>> sections;
    // Добавляем раздел в фиксированную последовательность отображения результатов.
    sections.push_back({
        "Название документа",
        {document_title.empty() ? "(пусто)" : document_title}
    });
    // Добавляем раздел в фиксированную последовательность отображения результатов.
    sections.push_back({"Метаданные", metadata});
    // Добавляем раздел в фиксированную последовательность отображения результатов.
    sections.push_back({"Заголовки разделов", headings});
    // Добавляем раздел в фиксированную последовательность отображения результатов.
    sections.push_back({"Текст страницы", page_text});
    // Добавляем раздел в фиксированную последовательность отображения результатов.
    sections.push_back({"Ссылки", links});
    // Добавляем раздел в фиксированную последовательность отображения результатов.
    sections.push_back({"Изображения", images});
    // Добавляем раздел в фиксированную последовательность отображения результатов.
    sections.push_back({"Элементы форм", form_fields});
    // Добавляем раздел в фиксированную последовательность отображения результатов.
    sections.push_back({"Канонический URL", canonical_urls});

    // Пустые разделы также показываются пользователю, чтобы структура результата оставалась одинаковой.
    for (auto& section : sections) {
        if (section.second.empty()) section.second.push_back("Список пуст.");
    }
    // Передаём вызывающему коду готовые разделы результата.
    return sections;
}

// Выводит статистику анализа HTML.
void print_analysis_statistics(const HtmlAnalysisResult& result) {
    // Выводим счётчики, накопленные парсером и валидатором, без повторного обхода токенов.
    std::cout << std::string(100, '=') << "\n";
    // Выводим очередную строку интерфейса или результат анализа.
    std::cout << "                                          РЕЗУЛЬТАТ АНАЛИЗА HTML\n";
    // Выводим очередную строку интерфейса или результат анализа.
    std::cout << "Открывающие теги:       " << result.opening_tags << "\n";
    // Выводим очередную строку интерфейса или результат анализа.
    std::cout << "Закрывающие теги:       " << result.closing_tags << "\n";
    // Выводим очередную строку интерфейса или результат анализа.
    std::cout << "Самозакрывающиеся теги: " << result.self_closing_tags << "\n";
    // Выводим очередную строку интерфейса или результат анализа.
    std::cout << "Атрибуты:               " << result.attributes << "\n";
    // Выводим очередную строку интерфейса или результат анализа.
    std::cout << "Комментарии:            " << result.comments << "\n";
    // Выводим очередную строку интерфейса или результат анализа.
    std::cout << "Текстовые фрагменты:    " << result.text_segments << "\n";
    // Выводим очередную строку интерфейса или результат анализа.
    std::cout << "Диагностики:            " << result.diagnostics.size() << "\n";
}

// Показывает выбранный раздел и значение с поддержкой клавиш-стрелок.
void show_document_information(
    const std::vector<std::pair<std::string, std::vector<std::string>>>& sections,
    std::size_t section_index,
    std::size_t value_index) {
    // Индексы раздела и значения предварительно ограничиваются навигационным циклом show_analysis_result().
    const auto& section = sections[section_index];
    const auto& values = section.second;

    // Выводим очередную строку интерфейса или результат анализа.
    std::cout << "\nИНФОРМАЦИЯ ИЗ ДОКУМЕНТА\n";
    // Выводим очередную строку интерфейса или результат анализа.
    std::cout << "Раздел " << section_index + 1 << " из "
              << sections.size() << ": " << section.first << "\n";
    // Выводим очередную строку интерфейса или результат анализа.
    std::cout << "Значение " << value_index + 1 << " из "
              << values.size() << "\n";
    // Выводим очередную строку интерфейса или результат анализа.
    std::cout << "\n  " << values[value_index] << "\n";
    // Выводим очередную строку интерфейса или результат анализа.
    std::cout << "\nСтрелки вверх/вниз — выбрать раздел.\n";
    // Выводим очередную строку интерфейса или результат анализа.
    std::cout << "Стрелки влево/вправо — переключить значение.\n";
    // Выводим очередную строку интерфейса или результат анализа.
    std::cout << "Enter или Esc — продолжить.\n";
}

// Выводит сообщения валидатора в порядке их обнаружения.
void print_diagnostics(const HtmlAnalysisResult& result) {
    // Если нарушений не найдено, выводим итоговое сообщение; иначе показываем каждую диагностику с координатами.
    if (result.diagnostics.empty()) {
        std::cout
            << "Проверка поддерживаемого подмножества HTML завершена успешно.\n";
        return;
    }

    for (const HtmlDiagnostic& diagnostic : result.diagnostics) {
        // Выводим очередную строку интерфейса или результат анализа.
        std::cout << "[" << diagnostic.category << "] строка "
                  << diagnostic.line << ", столбец "
                  << diagnostic.column << ": "
                  << utf8_to_console(diagnostic.message) << "\n";
    }
}


// Преобразует строку из кодировки консоли Windows-1251 в UTF-8 для текстового отчёта.
std::string console_to_utf8(const std::string& value) {
    // Отчёт хранится в UTF-8 независимо от кодировки, используемой консольным интерфейсом Windows.
    if (value.empty()) {
        // Возвращаем исходное значение, если безопасное преобразование выполнить не удалось.
        return value;
    }

    // Сначала преобразуем байты Windows-1251 в широкую строку Unicode.
    const int wide_size = MultiByteToWideChar(
        1251,
        0,
        value.data(),
        static_cast<int>(value.size()),
        nullptr,
        0);

    if (wide_size == 0) {
        // Возвращаем исходное значение, если безопасное преобразование выполнить не удалось.
        return value;
    }

    std::wstring wide_text(static_cast<std::size_t>(wide_size), L'\0');
    if (MultiByteToWideChar(
            1251,
            0,
            value.data(),
            static_cast<int>(value.size()),
            wide_text.data(),
            wide_size) == 0) {
        // Возвращаем исходное значение, если безопасное преобразование выполнить не удалось.
        return value;
    }

    // Затем кодируем Unicode в UTF-8 для записи в файл.
    const int utf8_size = WideCharToMultiByte(
        CP_UTF8,
        0,
        wide_text.data(),
        wide_size,
        nullptr,
        0,
        nullptr,
        nullptr);

    if (utf8_size == 0) {
        // Возвращаем исходное значение, если безопасное преобразование выполнить не удалось.
        return value;
    }

    std::string converted(static_cast<std::size_t>(utf8_size), '\0');
    if (WideCharToMultiByte(
            CP_UTF8,
            0,
            wide_text.data(),
            wide_size,
            converted.data(),
            utf8_size,
            nullptr,
            nullptr) == 0) {
        // Возвращаем исходное значение, если безопасное преобразование выполнить не удалось.
        return value;
    }

    // Возвращаем результат преобразования в целевую кодировку.
    return converted;
}

// Записывает статистику, извлечённые сведения и диагностики в UTF-8-файл.
bool save_analysis_report(
    const HtmlAnalysisResult& result,
    const std::string& source_filename,
    std::string& saved_path,
    std::string& error_message) {
    try {
        // Создаём каталог отчётов при первом сохранении, если его ещё нет.
        const fs::path output_directory("AnalysisResults");
        // Создаём каталог для отчётов, если он отсутствует.
        fs::create_directories(output_directory);

        const std::string base_name =
            fs::path(source_filename).stem().string() + "_analysis";
        // Формируем путь к файлу отчёта в каталоге результатов.
        fs::path report_path = output_directory / (base_name + ".txt");

        // Не перезаписываем ранее сохранённый отчёт с таким же именем.
        std::size_t suffix = 2;
        while (fs::exists(report_path)) {
            report_path = output_directory /
                (base_name + "_" + std::to_string(suffix) + ".txt");
            ++suffix;
        }

        // Бинарный режим исключает автоматическое преобразование переводов строк и сохраняет заданную кодировку.
        std::ofstream report(report_path, std::ios::binary);
        if (!report.is_open()) {
            error_message = "Не удалось открыть файл для записи.";
            // Сообщаем вызывающему коду о неуспешном завершении операции.
            return false;
        }

        // Маркер UTF-8 помогает редакторам корректно определить кодировку отчёта.
        report.write("\xEF\xBB\xBF", 3);
        // Записываем очередное поле отчёта, предварительно переводя текст в UTF-8.
        report << console_to_utf8("РЕЗУЛЬТАТ АНАЛИЗА HTML") << "\n";
        // Записываем разделитель для визуального оформления отчёта.
        report << std::string(60, '=') << "\n\n";
        // Записываем очередное поле отчёта, предварительно переводя текст в UTF-8.
        report << console_to_utf8("Исходный файл: ")
               << console_to_utf8(source_filename) << "\n\n";

        // Записываем очередное поле отчёта, предварительно переводя текст в UTF-8.
        report << console_to_utf8("СТАТИСТИКА") << "\n";
        // Записываем очередное поле отчёта, предварительно переводя текст в UTF-8.
        report << console_to_utf8("Открывающие теги: ")
               << result.opening_tags << "\n";
        // Записываем очередное поле отчёта, предварительно переводя текст в UTF-8.
        report << console_to_utf8("Закрывающие теги: ")
               << result.closing_tags << "\n";
        // Записываем очередное поле отчёта, предварительно переводя текст в UTF-8.
        report << console_to_utf8("Самозакрывающиеся теги: ")
               << result.self_closing_tags << "\n";
        // Записываем очередное поле отчёта, предварительно переводя текст в UTF-8.
        report << console_to_utf8("Атрибуты: ")
               << result.attributes << "\n";
        // Записываем очередное поле отчёта, предварительно переводя текст в UTF-8.
        report << console_to_utf8("Комментарии: ")
               << result.comments << "\n";
        // Записываем очередное поле отчёта, предварительно переводя текст в UTF-8.
        report << console_to_utf8("Текстовые фрагменты: ")
               << result.text_segments << "\n";
        // Записываем очередное поле отчёта, предварительно переводя текст в UTF-8.
        report << console_to_utf8("Диагностики: ")
               << result.diagnostics.size() << "\n";

        // Повторно используем общий сборщик данных, чтобы содержимое отчёта совпадало с разделами интерфейса.
        const auto sections = collect_document_information(result);
        for (const auto& section : sections) {
            report << "\n" << console_to_utf8(section.first) << "\n";
            // Записываем разделитель для визуального оформления отчёта.
            report << std::string(60, '-') << "\n";

            for (const std::string& value : section.second) {
                report << "- " << console_to_utf8(value) << "\n";
            }
        }

        // В конце отчёта сохраняем все сообщения проверки вместе с местом обнаружения.
        report << "\n" << console_to_utf8("ДИАГНОСТИКИ") << "\n";
        // Записываем разделитель для визуального оформления отчёта.
        report << std::string(60, '-') << "\n";
        if (result.diagnostics.empty()) {
            // Записываем очередное поле отчёта, предварительно переводя текст в UTF-8.
            report << console_to_utf8(
                "Проверка поддерживаемого подмножества HTML завершена успешно.")
                   << "\n";
        } else {
            for (const HtmlDiagnostic& diagnostic : result.diagnostics) {
                report << "[" << console_to_utf8(diagnostic.category) << "] "
                       << console_to_utf8("строка ") << diagnostic.line
                       << console_to_utf8(", столбец ") << diagnostic.column
                       << ": " << console_to_utf8(diagnostic.message) << "\n";
            }
        }

        // Принудительно отправляем буфер в файл и проверяем состояние потока до подтверждения успеха.
        report.flush();
        if (!report.good()) {
            error_message = "При записи отчёта произошла ошибка.";
            // Сообщаем вызывающему коду о неуспешном завершении операции.
            return false;
        }

        saved_path = report_path.string();
        // Подтверждаем успешное завершение операции.
        return true;
    } catch (const std::exception&) {
        error_message = "Не удалось сохранить отчёт из-за ошибки файловой системы.";
        // Сообщаем вызывающему коду о неуспешном завершении операции.
        return false;
    }
}

// Предлагает сохранить полный результат анализа в текстовый файл.
void show_save_prompt(
    const HtmlAnalysisResult& result,
    const std::string& source_filename) {
    // Повторяем показ меню, пока пользователь не выберет сохранение или возврат.
    while (true) {
        // Очищаем консоль перед отображением нового экрана.
        system("cls");
        // Выводим очередную строку интерфейса или результат анализа.
        std::cout << std::string(100, '=') << "\n";
        // Выводим очередную строку интерфейса или результат анализа.
        std::cout << "                                      СОХРАНЕНИЕ РЕЗУЛЬТАТА\n";
        // Выводим очередную строку интерфейса или результат анализа.
        std::cout << std::string(100, '=') << "\n";
        // Выводим очередную строку интерфейса или результат анализа.
        std::cout << "Сохранить результат анализа в текстовый файл?\n\n";
        // Выводим очередную строку интерфейса или результат анализа.
        std::cout << "1. Да, сохранить\n";
        // Выводим очередную строку интерфейса или результат анализа.
        std::cout << "2. Нет, вернуться в главное меню\n";
        // Выводим очередную строку интерфейса или результат анализа.
        std::cout << "Ваш выбор > ";

        // Считываем нажатие клавиши без подтверждения клавишей Enter.
        const int key = _getch();
        // Сохранение выполняется только после явного подтверждения пользователя.
        if (key == '1') {
            // Подготавливаем переменную для пути к сохранённому отчёту.
            std::string saved_path;
            // Подготавливаем текст для сообщения об ошибке сохранения.
            std::string error_message;

            // Очищаем консоль перед отображением нового экрана.
            system("cls");
            if (save_analysis_report(
                    result, source_filename, saved_path, error_message)) {
                // Выводим очередную строку интерфейса или результат анализа.
                std::cout << "Результат сохранён.\nФайл: "
                          << saved_path << "\n";
            } else {
                // Выводим очередную строку интерфейса или результат анализа.
                std::cout << error_message << "\n";
            }

            // Выводим очередную строку интерфейса или результат анализа.
            std::cout << "\nНажмите любую клавишу для возврата в меню...\n";
            input_data_reader(true, false, false);
            return;
        }

        // Клавиша Esc работает как альтернативный способ отказаться от сохранения.
        if (key == '2' || key == 27) {
            return;
        }
    }
}

}

// Отображает статистику, извлечённые сведения и диагностики HTML.
void show_analysis_result(
    const HtmlAnalysisResult& result,
    const std::string& source_filename) {
    // Подготавливаем разделы один раз и храним текущие позиции навигации между перерисовками экрана.
    const auto sections = collect_document_information(result);
    // Начинаем навигацию с первого раздела результата.
    std::size_t section_index = 0;
    // Начинаем просмотр с первого значения выбранного раздела.
    std::size_t value_index = 0;
    // Флаг управляет завершением цикла просмотра результата.
    bool finished = false;

    while (!finished) {
        // Очищаем консоль перед отображением нового экрана.
        system("cls");
        // Выводим сводные счётчики анализа перед подробными данными.
        print_analysis_statistics(result);
        // Показываем выбранный раздел и его текущее значение.
        show_document_information(sections, section_index, value_index);
        // Выводим очередную строку интерфейса или результат анализа.
        std::cout << "\n";
        // Выводим сообщения структурной проверки документа.
        print_diagnostics(result);

        // Считываем нажатие клавиши без подтверждения клавишей Enter.
        const int key = _getch();

        // _getch возвращает специальный префикс перед кодом клавиши-стрелки в Windows-консоли.
        if (key == 0 || key == 224) {
            // Считываем код стрелки после специального префикса клавиши.
            const int arrow = _getch();

            // Коды стрелок обновляют индекс раздела или индекс значения в текущем разделе.
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
