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


// Пропускаем пробельные символы и обновляем позицию курсора.
void skip_spaces(const std::string& text, std::size_t& cursor) { // text — анализируемая строка; cursor — позиция, которая будет обновлена.
    while (cursor < text.size() &&
           std::isspace(static_cast<unsigned char>(text[cursor]))) {
        ++cursor;
    }
}

// Находим закрывающий '>' вне значений, заключённых в кавычки.
std::size_t find_tag_end(const std::string& source, std::size_t start) { // source — исходный документ; start — позиция начала поиска.
    char quote = '\0';

    for (std::size_t cursor = start; cursor < source.size(); ++cursor) {
        const char ch = source[cursor];

        if (quote != '\0') {
            if (ch == quote) {
                quote = '\0';
            }
        } else if (ch == '"' || ch == '\'') {
            quote = ch;
        } else if (ch == '>') {
            return cursor;
        }
    }

    return std::string::npos; // Возвращаем npos, если закрывающий символ не найден.
}

// Добавляем диагностику с координатами исходного документа.
void add_diagnostic(
	HtmlAnalysisResult& result,     // Результат анализа, в который добавляется диагностика.
	const std::string& category,    // Категория проблемы, например "Лексическая ошибка" или "Структурная ошибка".
	const std::string& message,     // Сообщение, понятное пользователю, описывающее проблему.
	const PositionIndex& positions, // Объект, позволяющий определить номер строки по смещению в тексте файла.
	std::size_t offset) {           // Смещение в тексте файла, по которому нужно определить номер строки для диагностики.

    const std::size_t line = positions.at(offset);
    result.diagnostics.push_back({ category, message, line });
}

// Определяем самозакрытие
bool is_self_closing(const std::string& tag_string, std::size_t cursor) { // tag_string — содержимое тега без угловых скобок; cursor — позиция после имени тега.
    if (tag_string.empty() || tag_string.back() != '/') { // Если последний символ не '/', тег не самозакрывающийся.
        return false;
    }

    while (cursor < tag_string.size()) {
        skip_spaces(tag_string, cursor); // Пропускаем пробелы перед очередным атрибутом.

        if (cursor == tag_string.size() - 1) { // Последний слеш вне значения атрибута закрывает тег.
            return tag_string[cursor] == '/';
        }

        const std::size_t name_start = cursor;
        while (cursor < tag_string.size() && name_char(tag_string[cursor])) {
            ++cursor;
        }

        if (cursor == name_start) { // Продвигаемся через некорректный символ, чтобы цикл не застрял.
            ++cursor;
            continue;
        }

        skip_spaces(tag_string, cursor); // Между именем атрибута и '=' могут быть пробелы.
        if (cursor >= tag_string.size() || tag_string[cursor] != '=') {
            continue; // Имя атрибута уже прочитано, поэтому курсор продвинулся.
        }

        ++cursor;
        skip_spaces(tag_string, cursor); // Пропускаем пробелы после '='.

        if (cursor < tag_string.size() &&
            (tag_string[cursor] == '"' || tag_string[cursor] == '\'')) {
            const char quote = tag_string[cursor++]; // Запоминаем открывающую кавычку и переходим к значению.
            while (cursor < tag_string.size() && tag_string[cursor] != quote) {
                ++cursor;
            }

            if (cursor == tag_string.size()) {
                return false; // Незакрытое значение не позволяет подтвердить самозакрытие.
            }

            ++cursor; // Пропускаем закрывающую кавычку.
        } else {
            const std::size_t value_start = cursor;
            while (cursor < tag_string.size() &&
                   !std::isspace(static_cast<unsigned char>(tag_string[cursor]))) {
                ++cursor;
            }

            if (cursor == tag_string.size() && value_start < tag_string.size()) {
                return false; // Завершающий слеш поглощён некавыченным значением.
            }
        }
    }

    return true;
}

// Обрабатываем комментарий и возвращаем управление основному циклу.
bool parse_comment(
	const std::string& source,                      // Исходный HTML-документ, в котором ищем комментарий.
	std::size_t& cursor,                            // Позиция в исходной строке, с которой начинается поиск комментария; обновляется после обработки.
	HtmlToken& token,                               // Токен, в который сохраняется информация о найденном комментарии.
	HtmlAnalysisResult& result,                     // Результат анализа, в который добавляется токен комментария и при необходимости диагностика.
	const PositionIndex& positions) {               // Объект, позволяющий определить номер строки по смещению в тексте файла.

	if (source.compare(cursor, 4, "<!--") != 0) {           // Если текущая позиция не соответствует началу комментария, возвращаем false.
        return false;
    }

    const std::size_t start = cursor;                       //Сохраняем позицию начала комментария
	const std::size_t end = source.find("-->", cursor + 4); // Ищем первый закрывающий маркер комментария
	token.type = HtmlTokenType::Comment;                    // Устанавливаем тип токена как комментарий

    if (end == std::string::npos) {                         // Проверяем незакрытость комментария
        token.raw_text = source.substr(cursor);                 // Весь остаток исходного текста сохраняется как содержимое одного токена-комментария
		add_diagnostic(                                         // Добавляем диагностическое сообщение о незакрытом комментарии в результат анализа
            result,
            "Лексическая ошибка",
            "Для HTML-комментария ожидается завершающая последовательность -->.",
            positions,
            start);
        cursor = source.size();
    } else {
		cursor = end + 3;                                       // Сдвигаем курсор на позицию после закрывающего маркера комментария
		token.raw_text = source.substr(start, cursor - start);  // Сохраняем содержимое комментария в токен, включая открывающий и закрывающий маркеры
    }

	++result.comments;                                          // Увеличиваем счетчик комментариев в результате анализа
	result.tokens.push_back(token);                             // Добавляем токен комментария в список токенов результата анализа
    return true;
}

// Разбираем декларацию или инструкцию обработки с учётом кавычек.
bool parse_declaration(
    const std::string& source,              // Исходный HTML-документ.
    std::size_t& cursor,                    // Позиция начала декларации; обновляется после её обработки.
    HtmlToken& token,                       // Токен для сохранения декларации.
    HtmlAnalysisResult& result,             // Результат анализа, в который добавляется токен и диагностика.
    const PositionIndex& positions) {      // Объект для определения номера строки по смещению.
    if (source.compare(cursor, 2, "<!") != 0 &&
        source.compare(cursor, 2, "<?") != 0) {
        return false;
    }

    const std::size_t start = cursor;
    const std::size_t end = find_tag_end(source, cursor + 2); // Ищем '>' за начальным маркером декларации с учётом кавычек.

    token.type = HtmlTokenType::Declaration;    // Декларации и инструкции обработки хранятся целиком без дополнительного разбора.
    if (end == std::string::npos) {
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
    const std::string& source,          // Исходный HTML-документ.
    std::size_t& cursor,                // Позиция начала текста; обновляется до следующего '<' или конца документа.
    HtmlToken& token,                   // Токен, в который сохраняется текстовый фрагмент.
    HtmlAnalysisResult& result) {       // Результат анализа со списком токенов и счётчиком текстовых фрагментов.
    const std::size_t end = source.find('<', cursor);    // Текстовый фрагмент заканчивается перед следующим потенциальным тегом.
    cursor = end == std::string::npos ? source.size() : end;

    token.type = HtmlTokenType::Text;
    token.raw_text = source.substr(token.offset, cursor - token.offset);
    ++result.text_segments;
    result.tokens.push_back(token);
}

// Читаем атрибуты открывающего тега, сохраняя значения без ограничивающих кавычек.
void parse_attributes(
    const std::string& inside,              // Содержимое тега без внешних угловых скобок.
    std::size_t& cursor,                    // Позиция после имени тега; обновляется по мере чтения атрибутов.
    HtmlToken& token,                       // Токен, в который сохраняются найденные атрибуты.
    HtmlAnalysisResult& result,             // Результат анализа со счётчиком атрибутов и диагностикой.
    const PositionIndex& positions) {      // Объект для определения строки по смещению в исходном документе.
    while (cursor < inside.size()) {
        skip_spaces(inside, cursor); // Пропускаем пробелы между атрибутами.

        if (cursor < inside.size() && inside[cursor] == '/') {
            ++cursor; // Пропускаем разделительный слеш самозакрывающегося тега.
            continue;
        }

        if (cursor >= inside.size()) {
            break;
        }

        const std::size_t name_start = cursor;
        while (cursor < inside.size() && name_char(inside[cursor])) {
            ++cursor;
        }

        if (cursor == name_start) {
            ++cursor; // Пропускаем неожиданный символ, чтобы продолжить разбор.
            continue;
        }

        HtmlAttribute attribute;
        attribute.name = inside.substr(name_start, cursor - name_start); // Сохраняем имя атрибута.

        skip_spaces(inside, cursor);
        if (cursor < inside.size() && inside[cursor] == '=') {
            ++cursor;
            skip_spaces(inside, cursor); // Пропускаем пробелы перед значением.

            if (cursor < inside.size() &&
                (inside[cursor] == '"' || inside[cursor] == '\'')) {
                const char quote = inside[cursor++]; // Запоминаем тип кавычки и переходим к значению.
                const std::size_t value_start = cursor;

                while (cursor < inside.size() && inside[cursor] != quote) {
                    ++cursor;
                }

                attribute.value = inside.substr(value_start, cursor - value_start);
                if (cursor < inside.size()) {
                    ++cursor; // Пропускаем закрывающую кавычку.
                } else {
                    add_diagnostic(
                        result,
                        "Лексическая ошибка",
                        "Для значения атрибута ожидается закрывающая кавычка.",
                        positions,
                        token.offset);
                }
            } else {
                const std::size_t value_start = cursor;
                while (cursor < inside.size() &&
                       !std::isspace(static_cast<unsigned char>(inside[cursor]))) {
                    ++cursor;
                }
                attribute.value = inside.substr(value_start, cursor - value_start); // Сохраняем значение без кавычек.
            }
        }

        token.attributes.push_back(attribute); // Добавляем атрибут, даже если он задан без значения.
        ++result.attributes;
    }
}

// Разбираем тег целиком; при отсутствии > остаток документа считается текстом.
bool parse_tag(
    const std::string& source,              // Исходный HTML-документ.
    std::size_t& cursor,                    // Позиция начала тега; обновляется после его обработки.
    HtmlToken& token,                       // Токен, в который записываются данные тега.
    HtmlAnalysisResult& result,             // Результат анализа, содержащий токены, счётчики и диагностику.
    const PositionIndex& positions) {      // Объект для определения номера строки по смещению.
    const std::size_t start = cursor;
    const std::size_t end = find_tag_end(source, start + 1); // Ищем '>' с учётом кавычек внутри атрибутов.

    if (end == std::string::npos) {
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
    const std::string inside = source.substr(start + 1, end - start - 1); // Убираем внешние '<' и '>' для дальнейшего разбора.

    std::size_t name_cursor = 0;
    skip_spaces(inside, name_cursor);

    const bool closing = name_cursor < inside.size() && inside[name_cursor] == '/'; // Начальный слеш обозначает закрывающий тег.
    if (closing) {
        ++name_cursor;
        skip_spaces(inside, name_cursor);
    }

    const std::size_t name_start = name_cursor;
    while (name_cursor < inside.size() && name_char(inside[name_cursor])) {
        ++name_cursor;
    }

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

    token.name = lower_ascii(inside.substr(name_start, name_cursor - name_start)); // Нормализуем имя тега.

    if (closing) {
        std::size_t tail = name_cursor;
        skip_spaces(inside, tail);

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

    result.tokens.push_back(token);

    if (token.type == HtmlTokenType::OpenTag &&
        (token.name == "script" || token.name == "style")) {
        const std::size_t raw_end = find_raw_text_end(source, cursor, token.name); // Ищем закрывающий script/style, не разбирая содержимое как HTML.
        const std::size_t raw_start = cursor;
        const std::size_t raw_limit = raw_end == std::string::npos ? source.size() : raw_end;

        if (raw_start < raw_limit) {
            HtmlToken raw_token;
            raw_token.type = HtmlTokenType::Text;
            raw_token.offset = raw_start;
            raw_token.line = positions.at(raw_start);
            raw_token.raw_text = source.substr(raw_start, raw_limit - raw_start);

            result.tokens.push_back(raw_token);
            ++result.text_segments;
        }

        cursor = raw_limit;
    }

    return true;
}

// Последовательно преобразуем исходный HTML в токены и лексические диагностики.
void parse_html(const std::string& source, HtmlAnalysisResult& result) { // source — исходный HTML-документ; result — результат его разбора.
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
