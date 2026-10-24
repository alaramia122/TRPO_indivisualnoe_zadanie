// Подключаем общие объявления проекта.
#include "Header.h"

// Определяем номер строки по количеству символов перевода строки перед заданным смещением.
class PositionIndex {
public:
    PositionIndex(const std::string& source)                                    // source — исходный HTML-документ, по которому вычисляются номера строк.
        : source_(source) {}

    // Возвращаем номер строки, начиная отсчёт с единицы.
    std::size_t at(std::size_t offset) const {                                  // offset — смещение в исходном тексте, для которого нужно определить строку.

        offset = min(offset, source_.size());                                   // Ограничиваем смещение длиной исходного текста.
        return 1 + static_cast<std::size_t>(std::count(source_.begin(), source_.begin() + offset, '\n')); // Считаем переводы строк до смещения.
    }

private:
    const std::string& source_;
};

// Нормализуем регистр ASCII-имён тегов.
std::string lower_ascii(std::string value) {                                    // value — имя тега, которое нужно привести к нижнему регистру.
    for (char& ch : value) {                                                    // Перебираем каждый символ имени.
        ch = static_cast<char>(
            std::tolower(static_cast<unsigned char>(ch)));                      // Приводим символ к нижнему регистру безопасно для ctype.
    }
    return value;                                                               // Возвращаем строку с нормализованным именем.
}

// Ищем закрывающий тег script/style, не разбирая содержимое как HTML.
std::size_t find_raw_text_end(
    const std::string& source,                                                  // Исходный HTML-документ.
    std::size_t start,                                                          // Позиция, с которой начинается поиск.
    const std::string& tag_name) {                                              // Имя открытого тега: script или style.

    const std::string closing = "</" + tag_name;                                // Формируем искомый маркер закрывающего тега.

    for (std::size_t pos = start; pos + closing.size() <= source.size(); ++pos) { // Проверяем возможные позиции до конца документа.
        bool matches = true;                                                    // Предполагаем совпадение, пока не найдено различие.

        for (std::size_t j = 0; j < closing.size(); ++j) {                      // Сравниваем каждый символ искомого маркера.
            const char ch = source[pos + j];                                    // Получаем символ исходного текста на текущей позиции.

            if (static_cast<char>(std::tolower(static_cast<unsigned char>(ch))) != closing[j]) { // Сравниваем без учёта регистра.
                matches = false;                                                // Фиксируем несовпадение маркера.
                break;                                                          // Дальнейшее сравнение этой позиции не требуется.
            }
        }

        if (!matches) {                                                         // Если маркер не совпал, проверяем следующую позицию.
            continue;                                                           // Переходим к следующей позиции исходного текста.
        }

        const std::size_t after_name = pos + closing.size();                    // Находим символ сразу после имени тега.
        if (after_name == source.size() ||                                                      // Проверяем, завершилось ли имя закрывающего тега концом документа или допустимым разделителем.
            std::isspace(static_cast<unsigned char>(source[after_name])) ||     // Пробел означает завершение имени.
            source[after_name] == '>' ||                                        // Закрывающая скобка завершает тег.
            source[after_name] == '/') {                                        // Слеш может начинать завершающую часть тега.
            return pos;                                                         // Возвращаем позицию начала закрывающего тега.
        }
    }

    return std::string::npos;                                                   // Возвращаем npos, если закрывающий тег не найден.
}

// Проверяем символ, допустимый в имени тега или атрибута.
bool name_char(char value) {                                                    // value — символ, проверяемый на допустимость в имени тега или атрибута.
    return std::isalnum(static_cast<unsigned char>(value)) || value == '-' || value == '_' || // Проверяем, является ли символ буквой, цифрой или одним из разрешённых знаков имени.
           value == ':';
}


// Пропускаем пробельные символы и обновляем позицию курсора.
void skip_spaces(const std::string& text, std::size_t& cursor) {                // text — анализируемая строка; cursor — позиция, которая будет обновлена.
    while (cursor < text.size() &&                                      // Проверяем, что курсор не вышел за границы строки и текущий символ является пробельным.
           std::isspace(static_cast<unsigned char>(text[cursor]))) {            // Пока текущий символ является пробельным.
        ++cursor;                                                               // Переходим к следующему символу.
    }
}

// Находим закрывающий '>' вне значений, заключённых в кавычки.
std::size_t find_tag_end(const std::string& source, std::size_t start) {        // source — исходный документ; start — позиция начала поиска.
    char quote = '\0';                                                          // Нулевой символ означает, что сейчас мы не внутри кавычек.

    for (std::size_t cursor = start; cursor < source.size(); ++cursor) {        // Ищем закрывающую скобку до конца документа.
        const char ch = source[cursor];                                         // Считываем текущий символ.

        if (quote != '\0') {                                                    // Если кавычка открыта, '>' внутри значения не завершает тег.
            if (ch == quote) {                                                  // Проверяем, совпадает ли символ с закрывающей кавычкой.
                quote = '\0';                                                   // Закрывающая кавычка завершает значение.
            }
        }
        else if (ch == '"' || ch == '\'') {                                     // Запоминаем тип открывающей кавычки.
            quote = ch;                                                         // Обновляем состояние поиска внутри кавычек.
        }
        else if (ch == '>') {                                                   // Вне кавычек этот символ завершает тег или декларацию.
            return cursor;                                                      // Возвращаем индекс закрывающей скобки.
        }
    }

    return std::string::npos;                                                   // Возвращаем npos, если закрывающий символ не найден.
}

// Добавляем диагностику с координатами исходного документа.
void add_diagnostic(
	HtmlAnalysisResult& result,                                                    // Результат анализа, в который добавляется диагностика.
	const std::string& category,                                                   // Категория проблемы, например "Лексическая ошибка" или "Структурная ошибка".
	const std::string& message,                                                    // Сообщение, понятное пользователю, описывающее проблему.
	const PositionIndex& positions,                                                // Объект, позволяющий определить номер строки по смещению в тексте файла.
	std::size_t offset) {                                                          // Смещение в тексте файла, по которому нужно определить номер строки для диагностики.

    const std::size_t line = positions.at(offset);                              // Определяем строку, в которой находится проблемный фрагмент.
    result.diagnostics.push_back({ category, message, line });                  // Сохраняем категорию, сообщение и номер строки.
}

// Определяем самозакрытие
bool is_self_closing(const std::string& tag_string, std::size_t cursor) {       // tag_string — содержимое тега без угловых скобок; cursor — позиция после имени тега.
    if (tag_string.empty() || tag_string.back() != '/') {                       // Если последний символ не '/', тег не самозакрывающийся.
        return false;                                                           // Сообщаем вызывающей функции, что обработка не выполнена.
    }

    while (cursor < tag_string.size()) {                                        // Продолжаем проверять атрибуты, пока курсор не достигнет конца содержимого тега.
        skip_spaces(tag_string, cursor);                                        // Пропускаем пробелы перед очередным атрибутом.

        if (cursor == tag_string.size() - 1) {                                  // Последний слеш вне значения атрибута закрывает тег.
            return tag_string[cursor] == '/';                                   // Возвращаем результат проверки вызывающей функции.
        }

        const std::size_t name_start = cursor;                                  // Запоминаем начало фрагмента, чтобы затем корректно извлечь его из исходной строки.
        while (cursor < tag_string.size() &&                                // Продолжаем чтение значения до закрывающей кавычки или конца строки. name_char(tag_string[cursor])) {
            ++cursor;                                                           // Обновляем счётчик или позицию для перехода к следующему элементу.
        }

        if (cursor == name_start) {                                             // Проверяем, удалось ли считать хотя бы один символ имени.
            ++cursor;                                                           // Обновляем счётчик или позицию для перехода к следующему элементу.
            continue;                                                           // Переходим к следующей итерации цикла.
        }

        skip_spaces(tag_string, cursor);                                        // Между именем атрибута и '=' могут быть пробелы.
        if (cursor >= tag_string.size() || tag_string[cursor] != '=') {         // Проверяем, есть ли после имени атрибута знак равенства.
            continue;                                                           // Имя атрибута уже прочитано, поэтому курсор продвинулся.
        }

        ++cursor;                                                               // Обновляем счётчик или позицию для перехода к следующему элементу.
        skip_spaces(tag_string, cursor);                                        // Пропускаем пробелы после '='.

		if (cursor < tag_string.size() &&                                             // Если значение атрибута в кавычках, пропускаем его до закрывающей кавычки.
            (tag_string[cursor] == '"' || tag_string[cursor] == '\'')) {
            const char quote = tag_string[cursor++];                            // Запоминаем открывающую кавычку и переходим к значению.
            while (cursor < tag_string.size() && tag_string[cursor] != quote) { // Пропускаем значение до закрывающей кавычки.
                ++cursor;                                                       // Продолжаем поиск закрывающей кавычки.
            }

            if (cursor == tag_string.size()) {                                  // Проверяем, найдена ли закрывающая кавычка до конца строки.
                return false;                                                   // Незакрытое значение не позволяет подтвердить самозакрытие.
            }

            ++cursor;                                                           // Пропускаем закрывающую кавычку.
        }
    else {
            const std::size_t value_start = cursor;                             // Запоминаем начало фрагмента, чтобы затем корректно извлечь его из исходной строки.
            while (cursor < tag_string.size() &&                                // Ищем закрывающую кавычку, пока не достигнем конца строки.
                   !std::isspace(static_cast<unsigned char>(tag_string[cursor]))) { // Некавыченное значение продолжается до пробела.
                ++cursor;                                                       // Переходим к следующему символу значения.
            }

            if (cursor == tag_string.size() && value_start < tag_string.size()) { // Проверяем, дошло ли некавыченное значение до конца строки.
                return false;                                                   // Завершающий слеш поглощён некавыченным значением.
            }
        }
    }

    return true;                                                                // Сообщаем вызывающей функции об успешной обработке.
}

// Обрабатываем комментарий и возвращаем управление основному циклу.
bool parse_comment(
	const std::string& source,                                                     // Исходный HTML-документ, в котором ищем комментарий.
	std::size_t& cursor,                                                           // Позиция в исходной строке, с которой начинается поиск комментария; обновляется после обработки.
	HtmlToken& token,                                                              // Токен, в который сохраняется информация о найденном комментарии.
	HtmlAnalysisResult& result,                                                    // Результат анализа, в который добавляется токен комментария и при необходимости диагностика.
	const PositionIndex& positions) {                                              // Объект, позволяющий определить номер строки по смещению в тексте файла.

	if (source.compare(cursor, 4, "<!--") != 0) {                                  // Проверяем, начинается ли текст с маркера HTML-комментария.
        return false;                                                           // Сообщаем вызывающей функции, что обработка не выполнена.
    }

    const std::size_t start = cursor;                                           // Сохраняем позицию начала комментария
	const std::size_t end = source.find("-->", cursor + 4);                        // Ищем первый закрывающий маркер комментария
	token.type = HtmlTokenType::Comment;                                           // Устанавливаем тип токена как комментарий

    if (end == std::string::npos) {                                             // Проверяем, найден ли закрывающий символ '>'.
        token.raw_text = source.substr(cursor);                                 // Весь остаток исходного текста сохраняется как содержимое одного токена-комментария
		add_diagnostic(                                                               // Добавляем диагностическое сообщение о незакрытом комментарии в результат анализа
            result,
            "Лексическая ошибка",
            "Для HTML-комментария ожидается завершающая последовательность -->.",
            positions,
            start);
        cursor = source.size();                                                 // Обновляем позицию разбора, чтобы продолжить после обработанного фрагмента.
    }
    else {
		cursor = end + 3;                                                             // Сдвигаем курсор на позицию после закрывающего маркера комментария
		token.raw_text = source.substr(start, cursor - start);                        // Сохраняем содержимое комментария в токен, включая открывающий и закрывающий маркеры
    }

	++result.comments;                                                             // Увеличиваем счетчик комментариев в результате анализа
	result.tokens.push_back(token);                                                // Добавляем токен комментария в список токенов результата анализа
    return true;                                                                // Сообщаем вызывающей функции об успешной обработке.
}

// Разбираем декларацию или инструкцию обработки с учётом кавычек.
bool parse_declaration(
    const std::string& source,                                                  // Исходный HTML-документ.
    std::size_t& cursor,                                                        // Позиция начала декларации; обновляется после её обработки.
    HtmlToken& token,                                                           // Токен для сохранения декларации.
    HtmlAnalysisResult& result,                                                 // Результат анализа, в который добавляется токен и диагностика.
    const PositionIndex& positions) {                                           // Объект для определения номера строки по смещению.

    if (source.compare(cursor, 2, "<!") != 0 &&                                 // Проверяем, начинается ли фрагмент с маркера декларации или инструкции обработки.
        source.compare(cursor, 2, "<?") != 0) {
        return false;                                                           // Сообщаем вызывающей функции, что обработка не выполнена.
    }

    const std::size_t start = cursor;                                           // Запоминаем начало фрагмента, чтобы затем корректно извлечь его из исходной строки.
    const std::size_t end = find_tag_end(source, cursor + 2);                   // Ищем '>' за начальным маркером декларации с учётом кавычек.

    token.type = HtmlTokenType::Declaration;                                    // Сохраняем декларацию целиком, не разбирая её содержимое.
    if (end == std::string::npos) {                                             // Проверяем, найден ли закрывающий маркер комментария или декларации.
        token.raw_text = source.substr(cursor);                                 // Сохраняем исходный текст фрагмента без изменения содержимого.
        add_diagnostic(                                                         // Добавляем диагностику для некорректной или незавершённой конструкции.
            result,
            "Лексическая ошибка",
            "Для HTML-декларации ожидается завершающий символ >.",
            positions,
            start);
        cursor = source.size();                                                 // Обновляем позицию разбора, чтобы продолжить после обработанного фрагмента.
    }
    else {
        cursor = end + 1;                                                       // Обновляем позицию разбора, чтобы продолжить после обработанного фрагмента.
        token.raw_text = source.substr(start, cursor - start);                  // Сохраняем исходный текст фрагмента без изменения содержимого.
    }

    result.tokens.push_back(token);                                             // Добавляем сформированный токен в общий результат разбора.
    return true;                                                                // Сообщаем вызывающей функции об успешной обработке.
}

// Сохраняем обычный текст до следующего открывающего углового символа.
void parse_text(
    const std::string& source,                                                  // Исходный HTML-документ.
    std::size_t& cursor,                                                        // Позиция начала текста; обновляется до следующего '<' или конца документа.
    HtmlToken& token,                                                           // Токен, в который сохраняется текстовый фрагмент.
    HtmlAnalysisResult& result) {                                               // Результат анализа со списком токенов и счётчиком текстовых фрагментов.

    const std::size_t end = source.find('<', cursor);                           // Ищем следующий потенциальный тег, завершающий текстовый фрагмент.
    cursor = end == std::string::npos ? source.size() : end;                    // Обновляем позицию разбора, чтобы продолжить после обработанного фрагмента.

    token.type = HtmlTokenType::Text;                                           // Указываем тип найденного фрагмента.
    token.raw_text = source.substr(token.offset, cursor - token.offset);        // Сохраняем исходный текст фрагмента без изменения содержимого.
    ++result.text_segments;
    result.tokens.push_back(token);                                             // Добавляем сформированный токен в общий результат разбора.
}

// Читаем атрибуты открывающего тега, сохраняя значения без ограничивающих кавычек.
void parse_attributes(
    const std::string& inside,                                                  // Содержимое тега без внешних угловых скобок.
    std::size_t& cursor,                                                        // Позиция после имени тега; обновляется по мере чтения атрибутов.
    HtmlToken& token,                                                           // Токен, в который сохраняются найденные атрибуты.
    HtmlAnalysisResult& result,                                                 // Результат анализа со счётчиком атрибутов и диагностикой.
    const PositionIndex& positions) {                                           // Объект для определения строки по смещению в исходном документе.

    while (cursor < inside.size()) {                                // Продолжаем разбирать атрибуты до конца содержимого тега.
        skip_spaces(inside, cursor);                                            // Пропускаем пробелы между атрибутами.

        if (cursor < inside.size() &&                                   // Проверяем, остался ли символ в содержимом тега. inside[cursor] == '/') {
            ++cursor;                                                           // Пропускаем разделительный слеш самозакрывающегося тега.
            continue;                                                           // Переходим к следующей итерации цикла.
        }

        if (cursor >= inside.size()) {                                          // Проверяем, достиг ли курсор конца содержимого тега.
            break;                                                              // Завершаем цикл, поскольку дальнейший поиск не требуется.
        }

        const std::size_t name_start = cursor;                                  // Запоминаем начало имени атрибута.
        while (cursor < inside.size() &&                                // Читаем значение атрибута до пробельного символа или конца строки. name_char(inside[cursor])){
            ++cursor;                                                           // Переходим к следующему символу имени.
        }

        if (cursor == name_start) {                                             // Проверяем, удалось ли прочитать хотя бы один символ имени атрибута или тега.
            ++cursor;                                                           // Пропускаем неожиданный символ, чтобы продолжить разбор.
            continue;                                                           // Переходим к следующей итерации цикла.
        }

        HtmlAttribute attribute;                                                // Создаём структуру для сохранения атрибута.
        attribute.name = inside.substr(name_start, cursor - name_start);        // Сохраняем имя атрибута.

        skip_spaces(inside, cursor);                                            // Пропускаем пробельные символы перед следующим элементом.
        if (cursor < inside.size() &&                                           // Проверяем, есть ли символ после имени атрибута.
            ++cursor;                                                           // Обновляем счётчик или позицию для перехода к следующему элементу.
            skip_spaces(inside, cursor);                                        // Пропускаем пробелы перед значением.

            if (cursor < inside.size() &&                                       // Проверяем, начинается ли значение атрибута с кавычки.
                (inside[cursor] == '"' || inside[cursor] == '\'')) {
                const char quote = inside[cursor++];                            // Запоминаем тип кавычки и переходим к значению.
                const std::size_t value_start = cursor;                         // Запоминаем начало значения без открывающей кавычки.

                while (cursor < inside.size() && inside[cursor] != quote) {     // Ищем закрывающую кавычку.
                    ++cursor;                                                   // Продвигаемся по символам значения.
                }

                attribute.value = inside.substr(value_start, cursor - value_start); // Сохраняем значение без ограничивающих кавычек.
                if (cursor < inside.size()) {                                   // Проверяем, найдена ли закрывающая кавычка до конца строки.
                    ++cursor;                                                   // Пропускаем закрывающую кавычку.
                }
    else {
                    add_diagnostic(                                             // Добавляем диагностику для некорректной или незавершённой конструкции.
                        result,
                        "Лексическая ошибка",
                        "Для значения атрибута ожидается закрывающая кавычка.",
                        positions,
                        token.offset);                                          // Записываем извлечённые данные в текущий токен.
                }
            }
    else {
                const std::size_t value_start = cursor;                         // Запоминаем начало некавыченного значения.
                while (cursor < inside.size() &&                                // Читаем значение атрибута до пробельного символа или конца строки.
                       !std::isspace(static_cast<unsigned char>(inside[cursor]))) { // Читаем значение до пробельного символа.
                    ++cursor;                                                   // Продвигаемся по значению.
                }
                attribute.value = inside.substr(value_start, cursor - value_start); // Сохраняем значение без кавычек.
            }
        }

        token.attributes.push_back(attribute);                                  // Добавляем атрибут, даже если он задан без значения.
        ++result.attributes;                                                    // Обновляем общий счётчик атрибутов.
    }
}

// Разбираем тег целиком; при отсутствии > остаток документа считается текстом.
bool parse_tag(
    const std::string& source,                                                  // Исходный HTML-документ.
    std::size_t& cursor,                                                        // Позиция начала тега; обновляется после его обработки.
    HtmlToken& token,                                                           // Токен, в который записываются данные тега.
    HtmlAnalysisResult& result,                                                 // Результат анализа, содержащий токены, счётчики и диагностику.
    const PositionIndex& positions) {                                           // Объект для определения номера строки по смещению.
    const std::size_t start = cursor;                                           // Запоминаем начало тега для токена и диагностик.
    const std::size_t end = find_tag_end(source, start + 1);                    // Ищем '>' с учётом кавычек внутри атрибутов.

    if (end == std::string::npos) {                                             // Проверяем, найден ли закрывающий символ '>'.
        add_diagnostic(                                                         // Добавляем диагностику для некорректной или незавершённой конструкции.
            result,
            "Лексическая ошибка",
            "Для тега ожидается завершающий символ >.",
            positions,
            start);
        token.type = HtmlTokenType::Text;                                       // Указываем тип найденного фрагмента.
        token.raw_text = source.substr(start);                                  // Сохраняем исходный текст фрагмента без изменения содержимого.
        result.tokens.push_back(token);                                         // Добавляем сформированный токен в общий результат разбора.
        cursor = source.size();                                                 // Обновляем позицию разбора, чтобы продолжить после обработанного фрагмента.
        return false;                                                           // Сообщаем вызывающей функции, что обработка не выполнена.
    }

    cursor = end + 1;                                                           // Перемещаем курсор за весь найденный тег.
    token.raw_text = source.substr(start, cursor - start);                      // Сохраняем исходное представление тега.
    const std::string inside = source.substr(start + 1, end - start - 1);       // Убираем внешние '<' и '>' для дальнейшего разбора.

    std::size_t name_cursor = 0;                                                // Начинаем разбор с первого символа содержимого тега.
    skip_spaces(inside, name_cursor);                                           // Пропускаем пробелы перед именем или начальным слешем.

    const bool closing = name_cursor < inside.size() && inside[name_cursor] == '/'; // Начальный слеш обозначает закрывающий тег.
    if (closing) {                                                              // Проверяем, является ли текущий тег закрывающим.
        ++name_cursor;                                                          // Пропускаем слеш, обозначающий закрывающий тег.
        skip_spaces(inside, name_cursor);                                       // Пропускаем пробелы между слешем и именем.
    }

    const std::size_t name_start = name_cursor;                                 // Запоминаем начало имени тега.
    while (name_cursor < inside.size() && name_char(inside[name_cursor])) {     // Читаем имя до первого недопустимого символа.
        ++name_cursor;                                                          // Продвигаемся по имени.
    }

    if (name_cursor == name_start) {                                            // Проверяем, удалось ли прочитать имя тега.
        add_diagnostic(                                                         // Добавляем диагностику для некорректной или незавершённой конструкции.
            result,
            "Лексическая ошибка",
            "После символа < ожидается имя тега.",
            positions,
            start);
        token.type = HtmlTokenType::Text;                                       // Указываем тип найденного фрагмента.
        result.tokens.push_back(token);                                         // Добавляем сформированный токен в общий результат разбора.
        return true;                                                            // Сообщаем вызывающей функции об успешной обработке.
    }

    token.name = lower_ascii(inside.substr(name_start, name_cursor - name_start)); // Нормализуем имя тега.

    if (closing) {                                                              // Проверяем, является ли текущий тег закрывающим.
        std::size_t tail = name_cursor;                                         // Проверяем оставшуюся часть закрывающего тега.
        skip_spaces(inside, tail);                                              // После имени закрывающего тега разрешены только пробелы.

        if (tail != inside.size()) {                                            // Проверяем, остались ли после имени закрывающего тега лишние символы.
            add_diagnostic(                                                     // Добавляем диагностику для некорректной или незавершённой конструкции.
                result,
                "Лексическая ошибка",
                "После имени закрывающего тега обнаружены лишние символы.",
                positions,
                start);
            token.type = HtmlTokenType::Text;                                   // Указываем тип найденного фрагмента.
            result.tokens.push_back(token);                                     // Добавляем сформированный токен в общий результат разбора.
            return true;                                                        // Сообщаем вызывающей функции об успешной обработке.
        }

        token.type = HtmlTokenType::CloseTag;                                   // Указываем тип найденного фрагмента.
        ++result.closing_tags;
    }
    else {
        const bool self_closing = is_self_closing(inside, name_cursor);         // Проверяем завершающий слеш вне значений атрибутов.
        token.type = self_closing                                               // Указываем тип найденного фрагмента.
            ? HtmlTokenType::SelfClosingTag
            : HtmlTokenType::OpenTag;

        if (self_closing) {                                                     // Проверяем, заканчивается ли тег маркером самозакрытия.
            ++result.self_closing_tags;
        }
    else {
            ++result.opening_tags;
        }

        parse_attributes(inside, name_cursor, token, result, positions);        // Разбираем атрибуты открывающего тега.
    }

    result.tokens.push_back(token);                                             // Добавляем разобранный тег в результат.

    if (token.type == HtmlTokenType::OpenTag &&                                 // Проверяем, является ли тег открывающим script или style.
        (token.name == "script" || token.name == "style")) {
        const std::size_t raw_end = find_raw_text_end(source, cursor, token.name); // Ищем закрывающий script/style, не разбирая содержимое как HTML.
        const std::size_t raw_start = cursor;                                   // Сохраняем позицию сразу после открывающего тега.
        const std::size_t raw_limit = raw_end == std::string::npos ? source.size() : raw_end; // При отсутствии закрывающего тега берём остаток документа.

        if (raw_start < raw_limit) {                                            // Проверяем, есть ли непустое содержимое между открывающим и закрывающим тегами.
            HtmlToken raw_token;                                                // Создаём отдельный токен для содержимого script/style.
            raw_token.type = HtmlTokenType::Text;                               // Содержимое сохраняется как текст, а не как вложенная разметка.
            raw_token.offset = raw_start;                                       // Запоминаем начальное смещение фрагмента.
            raw_token.line = positions.at(raw_start);                           // Определяем строку начала фрагмента.
            raw_token.raw_text = source.substr(raw_start, raw_limit - raw_start); // Сохраняем содержимое до закрывающего тега.

            result.tokens.push_back(raw_token);                                 // Добавляем текстовый токен в результат.
            ++result.text_segments;                                             // Увеличиваем число текстовых фрагментов.
        }

        cursor = raw_limit;                                                     // Обновляем позицию разбора, чтобы продолжить после обработанного фрагмента.
    }

    return true;                                                                // Сообщаем вызывающей функции об успешной обработке.
}

// Последовательно преобразуем исходный HTML в токены и лексические диагностики.
void parse_html(const std::string& source, HtmlAnalysisResult& result) {        // source — исходный HTML-документ; result — результат его разбора.
    const PositionIndex positions(source);                                      // Создаём индекс строк один раз для всех токенов и диагностик.
    std::size_t cursor = 0;                                                     // Начинаем разбор с первого символа документа.

    while (cursor < source.size()) {                                            // На каждой итерации cursor должен продвигаться к следующему фрагменту документа.
        const std::size_t start = cursor;                                       // Сохраняем начало будущего токена.
        HtmlToken token;                                                        // Создаём новый токен для текущего фрагмента.
        token.offset = start;                                                   // Записываем смещение начала токена.
        token.line = positions.at(start);                                       // Определяем строку начала токена.

        if (parse_comment(source, cursor, token, result, positions)) {          // Сначала обрабатываем комментарии, чтобы их содержимое не разбиралось как теги.
            continue;                                                           // Переходим к следующей итерации цикла.
        }

        if (parse_declaration(source, cursor, token, result, positions)) {      // Проверяем, обработан ли текущий фрагмент как декларация или инструкция обработки.
            continue;                                                           // При успешном разборе переходим к следующему фрагменту.
        }

        if (source[cursor] != '<') {                                            // Если текущий символ не начинает HTML-конструкцию.
            parse_text(source, cursor, token, result);                          // Сохраняем текст до следующего '<'.
            continue;                                                           // Продолжаем разбор с новой позиции курсора.
        }

        if (!parse_tag(source, cursor, token, result, positions)) {             // Проверяем, удалось ли разобрать тег; при ошибке разбор документа завершается.
            break;                                                              // Дальнейший разбор невозможен после незакрытого тега.
        }
    }
}
