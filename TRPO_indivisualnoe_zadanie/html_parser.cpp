// Подключаем общие объявления проекта.
#include "Header.h"

namespace {

// Индекс координат позволяет получать строку и столбец без повторного просмотра документа.
class PositionIndex {
public:
    // Заранее строим таблицы координат, чтобы диагностики не пересчитывали весь префикс файла.
    explicit PositionIndex(const std::string& source)
        : codepoint_prefix_(source.size() + 1, 0) {
        line_starts_.push_back(0);

        // В UTF-8 один символ может занимать несколько байтов, поэтому столбец считаем по начальным байтам кодовых точек.
        for (std::size_t i = 0; i < source.size(); ++i) {
            const unsigned char ch = static_cast<unsigned char>(source[i]);
            const bool continuation = (ch & 0xC0) == 0x80;
            const bool newline = ch == '\r' || ch == '\n';

            codepoint_prefix_[i + 1] =
                codepoint_prefix_[i] + ((!continuation && !newline) ? 1u : 0u);

            // CRLF считается одним переводом строки; отдельно поддерживаются CR и LF.
            if (ch == '\r') {
                if (i + 1 < source.size() && source[i + 1] == '\n') {
                    line_starts_.push_back(i + 2);
                } else {
                    line_starts_.push_back(i + 1);
                }
            } else if (ch == '\n' &&
                       (i == 0 || source[i - 1] != '\r')) {
                line_starts_.push_back(i + 1);
            }
        }
    }

    // Возвращаем координаты с нумерацией строк и столбцов от единицы.
    std::pair<std::size_t, std::size_t> at(std::size_t offset) const {
        if (offset >= codepoint_prefix_.size()) {
            offset = codepoint_prefix_.size() - 1;
        }

        // Двоичный поиск находит последнюю строку, начавшуюся не позже заданного смещения.
        const auto it = std::upper_bound(
            line_starts_.begin(), line_starts_.end(), offset);
        const std::size_t line_index =
            static_cast<std::size_t>(it - line_starts_.begin() - 1);
        const std::size_t line_start = line_starts_[line_index];

        return {
            line_index + 1,
            codepoint_prefix_[offset] - codepoint_prefix_[line_start] + 1
        };
    }

private:
    // Байтовые смещения начала каждой строки исходного документа.
    std::vector<std::size_t> line_starts_;
    // Число UTF-8 кодовых точек до каждого байтового смещения.
    std::vector<std::size_t> codepoint_prefix_;
};

// Нормализуем регистр ASCII-имён тегов.
std::string lower_ascii(std::string value) {
    // Имена HTML-тегов сравниваются без учёта регистра.
    std::transform(
        value.begin(),
        value.end(),
        value.begin(),
        [](unsigned char ch) {
            return static_cast<char>(std::tolower(ch));
        });
    return value;
}

// Ищем закрывающий тег в script/style, не разбирая содержимое как HTML.
std::size_t find_raw_text_end(
    const std::string& source,
    std::size_t start,
    const std::string& tag_name) {
    // Ищем начало закрывающего тега, а не просто совпадение подстроки.
    const std::string closing = "</" + tag_name;

    for (std::size_t pos = start;
         pos + closing.size() <= source.size();
         ++pos) {
        bool matches = true;

        for (std::size_t j = 0; j < closing.size(); ++j) {
            const unsigned char ch =
                static_cast<unsigned char>(source[pos + j]);

            if (static_cast<char>(std::tolower(ch)) != closing[j]) {
                matches = false;
                break;
            }
        }

        if (!matches) {
            continue;
        }

        // После имени должен идти разделитель, иначе, например, </scripted> не завершает script.
        const std::size_t after_name = pos + closing.size();
        if (after_name == source.size() ||
            std::isspace(static_cast<unsigned char>(source[after_name])) ||
            source[after_name] == '>' ||
            source[after_name] == '/') {
            return pos;
        }
    }

    return std::string::npos;
}

// Проверяем символ, допустимый в имени тега или атрибута.
bool name_char(char ch) {
    // Помимо ASCII принимаем байты UTF-8, чтобы не отбрасывать не-латинские имена.
    const unsigned char value = static_cast<unsigned char>(ch);
    return std::isalnum(value) || ch == '-' || ch == '_' ||
           ch == ':' || value >= 128;
}

// Добавляем диагностику с координатами исходного документа.
void add_diagnostic(
    HtmlAnalysisResult& result,
    const std::string& category,
    const std::string& message,
    const PositionIndex& positions,
    std::size_t offset) {
    // Все сообщения используют один формат координат, рассчитанный PositionIndex.
    const auto position = positions.at(offset);
    result.diagnostics.push_back({
        category, message, position.first, position.second
    });
}

// Определяем самозакрытие, не принимая слеш внутри значения атрибута за маркер.
bool is_self_closing(const std::string& inside, std::size_t cursor) {
    // Слеш считается маркером только вне имени и значения атрибута.
    if (inside.empty() || inside.back() != '/') {
        return false;
    }

    while (cursor < inside.size()) {
        while (cursor < inside.size() &&
               std::isspace(static_cast<unsigned char>(inside[cursor]))) {
            ++cursor;
        }

        // Дошли до последнего слеша вне значения — тег самозакрывающийся.
        if (cursor == inside.size() - 1 && inside[cursor] == '/') {
            return true;
        }

        const std::size_t name_start = cursor;
        while (cursor < inside.size() && name_char(inside[cursor])) {
            ++cursor;
        }

        if (cursor == name_start) {
            ++cursor;
            continue;
        }

        while (cursor < inside.size() &&
               std::isspace(static_cast<unsigned char>(inside[cursor]))) {
            ++cursor;
        }

        // Атрибут без '=' является булевым и значения не имеет.
        if (cursor >= inside.size() || inside[cursor] != '=') {
            continue;
        }

        ++cursor;
        while (cursor < inside.size() &&
               std::isspace(static_cast<unsigned char>(inside[cursor]))) {
            ++cursor;
        }

        // Значение в кавычках пропускаем целиком: слеш внутри него не завершает тег.
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
            const std::size_t value_start = cursor;
            while (cursor < inside.size() &&
                   !std::isspace(static_cast<unsigned char>(inside[cursor]))) {
                ++cursor;
            }

            if (cursor == inside.size() && value_start < inside.size()) {
                return false;
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

    // Комментарий сохраняется единым токеном, чтобы его содержимое не разбиралось как теги.
    const std::size_t start = cursor;
    const std::size_t end = source.find("-->", cursor + 4);
    token.type = HtmlTokenType::Comment;

    // Незакрытый комментарий поглощает остаток файла и сопровождается диагностикой.
    if (end == std::string::npos) {
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

    // Символ '>' внутри кавычек не завершает декларацию.
    const std::size_t start = cursor;
    bool single_quote = false;
    bool double_quote = false;
    std::size_t end = cursor + 2;

    for (; end < source.size(); ++end) {
        const char ch = source[end];

        // Отслеживаем кавычки раздельно, чтобы не спутать вложенную кавычку с закрывающей.
        if (ch == '\'' && !double_quote) {
            single_quote = !single_quote;
        } else if (ch == '"' && !single_quote) {
            double_quote = !double_quote;
        } else if (ch == '>' && !single_quote && !double_quote) {
            break;
        }
    }

    // Декларации и инструкции обработки хранятся целиком без дополнительного разбора.
    token.type = HtmlTokenType::Declaration;
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
    // Текстовый фрагмент заканчивается перед следующим потенциальным тегом.
    const std::size_t end = source.find('<', cursor);
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
    // Курсор движется по содержимому тега, не включая внешние символы '<' и '>'.
    while (cursor < inside.size()) {
        // Пробелы и разделительный слеш не являются частью имени атрибута.
        while (cursor < inside.size() &&
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

        // Сначала фиксируем имя, затем при наличии '=' читаем значение.
        HtmlAttribute attribute;
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

            // Для quoted-значения ограничители не входят в attribute.value.
            if (cursor < inside.size() &&
                (inside[cursor] == '"' || inside[cursor] == '\'')) {
                const char quote = inside[cursor++];
                const std::size_t value_start = cursor;

                // Ищем закрывающую кавычку того же типа, что и открывающая.
                while (cursor < inside.size() && inside[cursor] != quote) {
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
                // Некавыченное значение читается до пробела или конца содержимого тега.
                const std::size_t value_start = cursor;
                while (cursor < inside.size() &&
                       !std::isspace(static_cast<unsigned char>(inside[cursor]))) {
                    ++cursor;
                }
                attribute.value = inside.substr(value_start, cursor - value_start);
            }
        }

        // Счётчик отражает число распознанных атрибутов, включая атрибуты без значения.
        token.attributes.push_back(attribute);
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
    // Сначала ищем закрывающий '>' вне кавычек, чтобы не обрезать значения атрибутов.
    const std::size_t start = cursor;
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

    // Незавершённый тег диагностируется; оставшаяся часть файла сохраняется как текст.
    if (end == source.size()) {
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
    // Дальнейший разбор выполняется по содержимому без внешних угловых скобок.
    const std::string inside = source.substr(start + 1, end - start - 1);

    std::size_t name_cursor = 0;
    while (name_cursor < inside.size() &&
           std::isspace(static_cast<unsigned char>(inside[name_cursor]))) {
        ++name_cursor;
    }

    // Начальный слеш отделяет закрывающий тег от открывающего.
    const bool closing =
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

    // Если имя отсутствует, сохраняем исходную конструкцию как текст и выдаём диагностику.
    if (name_cursor == name_start) {
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

    // У закрывающего тега после имени допускаются только пробельные символы.
    if (closing) {
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
        // Для открывающего тега отдельно определяем самозакрытие и разбираем атрибуты.
        const bool self_closing = is_self_closing(inside, name_cursor);
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

    // Сначала добавляем сам тег, затем при необходимости — отдельный токен его сырого содержимого.
    result.tokens.push_back(token);

    if (token.type == HtmlTokenType::OpenTag &&
        (token.name == "script" || token.name == "style")) {
        // Внутри script/style символы '<' относятся к коду, поэтому обычный разбор текста отключён.
        const std::size_t raw_end = find_raw_text_end(source, cursor, token.name);
        const std::size_t raw_start = cursor;
        const std::size_t raw_limit =
            raw_end == std::string::npos ? source.size() : raw_end;

        // Не создаём пустой текстовый токен, если содержимого между тегами нет.
        if (raw_start < raw_limit) {
            HtmlToken raw_token;
            raw_token.type = HtmlTokenType::Text;
            raw_token.offset = raw_start;

            const auto raw_position = positions.at(raw_start);
            raw_token.line = raw_position.first;
            raw_token.column = raw_position.second;
            raw_token.raw_text = source.substr(raw_start, raw_limit - raw_start);

            result.tokens.push_back(raw_token);
            ++result.text_segments;
        }

        cursor = raw_end == std::string::npos ? source.size() : raw_end;
    }

    return true;
}

} // namespace

// Последовательно преобразуем исходный HTML в токены и лексические диагностики.
void parse_html(const std::string& source, HtmlAnalysisResult& result) {
    // Подготавливаем координаты один раз и используем их для всех токенов и диагностик.
    const PositionIndex positions(source);
    std::size_t cursor = 0;

    // На каждой итерации cursor должен продвигаться к следующему фрагменту документа.
    while (cursor < source.size()) {
        const std::size_t start = cursor;
        const auto position = positions.at(start);

        // Позиция токена фиксируется до разбора, пока cursor указывает на его первый байт.
        HtmlToken token;
        token.offset = start;
        token.line = position.first;
        token.column = position.second;

        // Проверяем более специфичные конструкции раньше обычного тега или текста.
        if (parse_comment(source, cursor, token, result, positions)) {
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
