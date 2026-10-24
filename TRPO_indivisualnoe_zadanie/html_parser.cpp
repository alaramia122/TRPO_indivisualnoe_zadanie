// Подключаем общие объявления проекта.
#include "Header.h"

// Считаем строки по символу перевода строки \n.
class PositionIndex {
public:
    PositionIndex(const std::string& source)
        : source_(source) {}

    // Возвращаем номер строки, начиная с единицы.
    std::size_t at(std::size_t offset) const {

        offset = min(offset, source_.size());    //контроллируем, чтобы offset не выходил за пределы строки
        return 1 + static_cast<std::size_t>(std::count(source_.begin(), source_.begin() + offset, '\n'));    //считаем количество символов перевода строки
    }    //до указанного смещения и прибавляем 1

private:
    const std::string& source_;
};

// Нормализуем регистр ASCII-имён тегов.
std::string lower_ascii(std::string value) {
    for (char& ch : value) {
        ch = static_cast<char>(
            std::tolower(static_cast<unsigned char>(ch)));
    }
    return value;
}

std::size_t find_raw_text_end(    // Ищем закрывающий тег в script/style, не разбирая содержимое как HTML.
    const std::string& source,    //весь исходный HTML-документ
    std::size_t start,    //позиция, с которой нужно начинать поиск
    const std::string& tag_name) {    //имя тега

    const std::string closing = "</" + tag_name;    // Ищем начало закрывающего тега, а не просто совпадение подстроки.

    for (std::size_t pos = start; pos + closing.size() <= source.size(); ++pos) {    //Проверяем все позиции в исходной строке, начиная с позиции start и до конца строки

        bool matches = true;

        for (std::size_t j = 0; j < closing.size(); ++j) {    //Сравниваем символы исходного документа с искомой строкой

            char ch = source[pos + j];

            if (static_cast<char>(std::tolower(static_cast<unsigned char>(ch))) != closing[j]) {
                matches = false;
                break;
            }
        }

        if (!matches) {
            continue;
        }

        const std::size_t after_name = pos + closing.size();    // После имени должен идти разделитель
        if (after_name == source.size() ||    // Имя заканчивается в конце строки
            std::isspace(static_cast<unsigned char>(source[after_name])) ||    // После имени идёт пробельный символ
            source[after_name] == '>' ||    // После имени идёт символ '>'
            source[after_name] == '/') {    // После имени идёт символ '/'
            return pos;
        }
    }

    return std::string::npos;
}

// Проверяем символ, допустимый в имени тега или атрибута.
bool name_char(char value) {
    return std::isalnum(static_cast<unsigned char>(value)) || value == '-' || value == '_' ||
           value == ':';
}

// Добавляем диагностику с координатами исходного документа.
void add_diagnostic(
    HtmlAnalysisResult& result,
    const std::string& category,
    const std::string& message,
    const PositionIndex& positions,
    std::size_t offset) {

    const std::size_t line = positions.at(offset);
    result.diagnostics.push_back({ category, message, line });
}

// Определяем самозакрытие, не принимая слеш внутри значения атрибута за маркер.
bool is_self_closing(const std::string& inside, std::size_t cursor) {
    if (inside.empty() || inside.back() != '/') {
        return false;
    }

    while (cursor < inside.size()) {
        while (cursor < inside.size() &&
               std::isspace(static_cast<unsigned char>(inside[cursor]))) {
            ++cursor;
        }

        if (cursor == inside.size() - 1) {
            return inside[cursor] == '/';
        }

        while (cursor < inside.size() && name_char(inside[cursor])) {
            ++cursor;
        }

        while (cursor < inside.size() &&
               std::isspace(static_cast<unsigned char>(inside[cursor]))) {
            ++cursor;
        }

        if (cursor >= inside.size() || inside[cursor] != '=') {
            if (cursor < inside.size()) {
                ++cursor;
            }
            continue;
        }

        ++cursor;

        while (cursor < inside.size() &&
               std::isspace(static_cast<unsigned char>(inside[cursor]))) {
            ++cursor;
        }

        if (cursor < inside.size() &&
            (inside[cursor] == '"' || inside[cursor] == '\'')) {
            const char quote = inside[cursor++];
            while (cursor < inside.size() && inside[cursor] != quote) {
                ++cursor;
            }

            if (cursor < inside.size()) {
                ++cursor;
            }
        } else {
            while (cursor < inside.size() &&
                   !std::isspace(static_cast<unsigned char>(inside[cursor]))) {
                ++cursor;
            }
        }
    }

    return true;
}

// Обрабатываем комментарий и возвращаем управление основному циклу.
bool parse_comment(
    const std::string& source,
    std::size_t& cursor,
    HtmlToken& token,
    HtmlAnalysisResult& result,
    const PositionIndex& positions) {
    if (source.compare(cursor, 4, "<!--") != 0) {
        return false;
    }

    const std::size_t start = cursor;    // Комментарий сохраняется единым токеном, чтобы его содержимое не разбиралось как теги.
    const std::size_t end = source.find("-->", cursor + 4);
    token.type = HtmlTokenType::Comment;

    if (end == std::string::npos) {    // Незакрытый комментарий поглощает остаток файла и сопровождается диагностикой.
        token.raw_text = source.substr(cursor);
        add_diagnostic(
            result,
            "Лексическая ошибка",
            "Для HTML-комментария ожидается завершающая последовательность -->.",
            positions,
            start);
        cursor = source.size();
    } else {
        cursor = end + 3;
        token.raw_text = source.substr(start, cursor - start);
    }

    ++result.comments;
    result.tokens.push_back(token);
    return true;
}

// Разбираем декларацию или инструкцию обработки с учётом кавычек.
bool parse_declaration(
    const std::string& source,
    std::size_t& cursor,
    HtmlToken& token,
    HtmlAnalysisResult& result,
    const PositionIndex& positions) {
    if (source.compare(cursor, 2, "<!") != 0 &&
        source.compare(cursor, 2, "<?") != 0) {
        return false;
    }

    const std::size_t start = cursor;    // Символ '>' внутри кавычек не завершает декларацию.
    bool single_quote = false;
    bool double_quote = false;
    std::size_t end = cursor + 2;

    for (; end < source.size(); ++end) {
        const char ch = source[end];

        if (ch == '\'' && !double_quote) {    // Отслеживаем кавычки раздельно, чтобы не спутать вложенную кавычку с закрывающей.
            single_quote = !single_quote;
        } else if (ch == '"' && !single_quote) {
            double_quote = !double_quote;
        } else if (ch == '>' && !single_quote && !double_quote) {
            break;
        }
    }

    token.type = HtmlTokenType::Declaration;    // Декларации и инструкции обработки хранятся целиком без дополнительного разбора.
    if (end >= source.size()) {
        token.raw_text = source.substr(cursor);
        add_diagnostic(
            result,
            "Лексическая ошибка",
            "Для HTML-декларации ожидается завершающий символ >.",
            positions,
            start);
        cursor = source.size();
    } else {
        cursor = end + 1;
        token.raw_text = source.substr(start, cursor - start);
    }

    result.tokens.push_back(token);
    return true;
}

// Сохраняем обычный текст до следующего открывающего углового символа.
void parse_text(
    const std::string& source,
    std::size_t& cursor,
    HtmlToken& token,
    HtmlAnalysisResult& result) {
    const std::size_t end = source.find('<', cursor);    // Текстовый фрагмент заканчивается перед следующим потенциальным тегом.
    cursor = end == std::string::npos ? source.size() : end;

    token.type = HtmlTokenType::Text;
    token.raw_text = source.substr(token.offset, cursor - token.offset);
    ++result.text_segments;
    result.tokens.push_back(token);
}

// Читаем атрибуты открывающего тега, сохраняя значения без ограничивающих кавычек.
void parse_attributes(
    const std::string& inside,
    std::size_t& cursor,
    HtmlToken& token,
    HtmlAnalysisResult& result,
    const PositionIndex& positions) {
    while (cursor < inside.size()) {    // Курсор движется по содержимому тега, не включая внешние символы '<' и '>'.
        while (cursor < inside.size() &&    // Пробелы и разделительный слеш не являются частью имени атрибута.
               (std::isspace(static_cast<unsigned char>(inside[cursor])) ||
                inside[cursor] == '/')) {
            ++cursor;
        }

        if (cursor >= inside.size()) {
            break;
        }

        const std::size_t name_start = cursor;
        while (cursor < inside.size() && name_char(inside[cursor])) {
            ++cursor;
        }

        if (cursor == name_start) {
            ++cursor;
            continue;
        }

        HtmlAttribute attribute;    // Сначала фиксируем имя, затем при наличии '=' читаем значение.
        attribute.name = inside.substr(name_start, cursor - name_start);

        while (cursor < inside.size() &&
               std::isspace(static_cast<unsigned char>(inside[cursor]))) {
            ++cursor;
        }

        if (cursor < inside.size() && inside[cursor] == '=') {
            ++cursor;
            while (cursor < inside.size() &&
                   std::isspace(static_cast<unsigned char>(inside[cursor]))) {
                ++cursor;
            }

            if (cursor < inside.size() &&    // Для quoted-значения ограничители не входят в attribute.value.
                (inside[cursor] == '"' || inside[cursor] == '\'')) {
                const char quote = inside[cursor++];
                const std::size_t value_start = cursor;

                while (cursor < inside.size() && inside[cursor] != quote) {    // Ищем закрывающую кавычку того же типа, что и открывающая.
                    ++cursor;
                }

                attribute.value = inside.substr(value_start, cursor - value_start);
                if (cursor < inside.size()) {
                    ++cursor;
                } else {
                    add_diagnostic(
                        result,
                        "Лексическая ошибка",
                        "Для значения атрибута ожидается закрывающая кавычка.",
                        positions,
                        token.offset);
                }
            } else {
                const std::size_t value_start = cursor;    // Некавыченное значение читается до пробела или конца содержимого тега.
                while (cursor < inside.size() &&
                       !std::isspace(static_cast<unsigned char>(inside[cursor]))) {
                    ++cursor;
                }
                attribute.value = inside.substr(value_start, cursor - value_start);
            }
        }

        token.attributes.push_back(attribute);    // Счётчик отражает число распознанных атрибутов, включая атрибуты без значения.
        ++result.attributes;
    }
}

// Разбираем тег целиком; при отсутствии > остаток документа считается текстом.
bool parse_tag(
    const std::string& source,
    std::size_t& cursor,
    HtmlToken& token,
    HtmlAnalysisResult& result,
    const PositionIndex& positions) {
    const std::size_t start = cursor;    // Сначала ищем закрывающий '>' вне кавычек, чтобы не обрезать значения атрибутов.
    bool single_quote = false;
    bool double_quote = false;
    std::size_t end = cursor + 1;

    for (; end < source.size(); ++end) {
        const char ch = source[end];

        if (ch == '\'' && !double_quote) {
            single_quote = !single_quote;
        } else if (ch == '"' && !single_quote) {
            double_quote = !double_quote;
        } else if (ch == '>' && !single_quote && !double_quote) {
            break;
        }
    }

    if (end == source.size()) {    // Незавершённый тег диагностируется; оставшаяся часть файла сохраняется как текст.
        add_diagnostic(
            result,
            "Лексическая ошибка",
            "Для тега ожидается завершающий символ >.",
            positions,
            start);
        token.type = HtmlTokenType::Text;
        token.raw_text = source.substr(start);
        result.tokens.push_back(token);
        cursor = source.size();
        return false;
    }

    cursor = end + 1;
    token.raw_text = source.substr(start, cursor - start);
    const std::string inside = source.substr(start + 1, end - start - 1);    // Дальнейший разбор выполняется по содержимому без внешних угловых скобок.

    std::size_t name_cursor = 0;
    while (name_cursor < inside.size() &&
           std::isspace(static_cast<unsigned char>(inside[name_cursor]))) {
        ++name_cursor;
    }

    const bool closing =    // Начальный слеш отделяет закрывающий тег от открывающего.
        name_cursor < inside.size() && inside[name_cursor] == '/';
    if (closing) {
        ++name_cursor;
    }

    while (name_cursor < inside.size() &&
           std::isspace(static_cast<unsigned char>(inside[name_cursor]))) {
        ++name_cursor;
    }

    const std::size_t name_start = name_cursor;
    while (name_cursor < inside.size() && name_char(inside[name_cursor])) {
        ++name_cursor;
    }

    if (name_cursor == name_start) {    // Если имя отсутствует, сохраняем исходную конструкцию как текст и выдаём диагностику.
        add_diagnostic(
            result,
            "Лексическая ошибка",
            "После символа < ожидается имя тега.",
            positions,
            start);
        token.type = HtmlTokenType::Text;
        result.tokens.push_back(token);
        return true;
    }

    token.name = lower_ascii(inside.substr(name_start, name_cursor - name_start));

    if (closing) {    // У закрывающего тега после имени допускаются только пробельные символы.
        std::size_t tail = name_cursor;
        while (tail < inside.size() &&
               std::isspace(static_cast<unsigned char>(inside[tail]))) {
            ++tail;
        }

        if (tail != inside.size()) {
            add_diagnostic(
                result,
                "Лексическая ошибка",
                "После имени закрывающего тега обнаружены лишние символы.",
                positions,
                start);
            token.type = HtmlTokenType::Text;
            result.tokens.push_back(token);
            return true;
        }

        token.type = HtmlTokenType::CloseTag;
        ++result.closing_tags;
    } else {
        const bool self_closing = is_self_closing(inside, name_cursor);    // Для открывающего тега отдельно определяем самозакрытие и разбираем атрибуты.
        token.type = self_closing
            ? HtmlTokenType::SelfClosingTag
            : HtmlTokenType::OpenTag;

        if (self_closing) {
            ++result.self_closing_tags;
        } else {
            ++result.opening_tags;
        }

        parse_attributes(inside, name_cursor, token, result, positions);
    }

    result.tokens.push_back(token);    // Сначала добавляем сам тег, затем при необходимости — отдельный токен его сырого содержимого.

    if (token.type == HtmlTokenType::OpenTag &&
        (token.name == "script" || token.name == "style")) {
        const std::size_t raw_end = find_raw_text_end(source, cursor, token.name);    // Внутри script/style символы '<' относятся к коду, поэтому обычный разбор текста отключён.
        const std::size_t raw_start = cursor;
        const std::size_t raw_limit =
            raw_end == std::string::npos ? source.size() : raw_end;

        if (raw_start < raw_limit) {    // Не создаём пустой текстовый токен, если содержимого между тегами нет.
            HtmlToken raw_token;
            raw_token.type = HtmlTokenType::Text;
            raw_token.offset = raw_start;

            raw_token.line = positions.at(raw_start);
            raw_token.raw_text = source.substr(raw_start, raw_limit - raw_start);

            result.tokens.push_back(raw_token);
            ++result.text_segments;
        }

        cursor = raw_end == std::string::npos ? source.size() : raw_end;
    }

    return true;
}

// Последовательно преобразуем исходный HTML в токены и лексические диагностики.
void parse_html(const std::string& source, HtmlAnalysisResult& result) {
    const PositionIndex positions(source);    // Подготавливаем координаты один раз и используем их для всех токенов и диагностик.
    std::size_t cursor = 0;

    while (cursor < source.size()) {    // На каждой итерации cursor должен продвигаться к следующему фрагменту документа.
        const std::size_t start = cursor;
        HtmlToken token;    // Позиция токена фиксируется до разбора, пока cursor указывает на его первый байт.
        token.offset = start;
        token.line = positions.at(start);

        if (parse_comment(source, cursor, token, result, positions)) {    // Проверяем более специфичные конструкции раньше обычного тега или текста.
            continue;
        }

        if (parse_declaration(source, cursor, token, result, positions)) {
            continue;
        }

        if (source[cursor] != '<') {
            parse_text(source, cursor, token, result);
            continue;
        }

        if (!parse_tag(source, cursor, token, result, positions)) {
            break;
        }
    }
}
