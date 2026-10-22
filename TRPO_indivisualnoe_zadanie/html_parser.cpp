#include "Header.h" // Подключаем общие объявления.
namespace { // Ограничиваем вспомогательные функции текущим файлом.
using Position = std::pair<std::size_t, std::size_t>; // Представляем координаты строки и столбца.
std::vector<Position> build_positions(const std::string& source) { // Рассчитываем координаты всех смещений за один проход.
    std::vector<Position> positions(source.size() + 1); // Выделяем координаты для каждого байтового смещения.
    std::size_t line = 1, column = 1, i = 0; // Начинаем отсчёт с первой строки и первого столбца.
    while (i < source.size()) { // Обрабатываем исходный текст последовательно.
        positions[i] = { line, column }; // Сохраняем координаты текущего байта.
        if (source[i] == '\r') { // Обрабатываем перевод строки CR или CRLF.
            if (i + 1 < source.size() && source[i + 1] == '\n') { positions[i + 1] = { line, column }; i += 2; } // Считаем CRLF одним переводом строки.
            else ++i; // Пропускаем одиночный CR.
            ++line; column = 1; positions[i] = { line, column }; // Переходим к началу следующей строки.
        }
        else if (source[i] == '\n') { ++line; column = 1; ++i; positions[i] = { line, column }; } // Обрабатываем одиночный LF.
        else { const unsigned char ch = static_cast<unsigned char>(source[i]); if ((ch & 0xC0) != 0x80) ++column; ++i; positions[i] = { line, column }; } // Считаем UTF-8 символ по ведущему байту, а не по каждому байту.
    }
    return positions; // Возвращаем таблицу координат.
} // Завершаем построение таблицы координат.
std::string lower_ascii(std::string value) { // Нормализуем имя тега.
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); }); // Переводим латиницу в нижний регистр.
    return value; // Возвращаем нормализованную строку.
} // Завершаем нормализацию.
bool name_char(char ch) { const unsigned char c = static_cast<unsigned char>(ch); return std::isalnum(c) || ch == '-' || ch == '_' || ch == ':' || c >= 128; } // Проверяем символ имени тега или атрибута.
void diagnostic(HtmlAnalysisResult& result, const std::string& category, const std::string& message, const std::vector<Position>& positions, std::size_t offset) { const auto p = positions[std::min(offset, positions.size() - 1)]; result.diagnostics.push_back({ category, message, p.first, p.second }); } // Добавляем сообщение с предварительно рассчитанными координатами.
bool is_self_closing(const std::string& inside, std::size_t name_end, std::size_t tail) { // Отличаем синтаксический слеш от слеша в некавыченном значении.
    if (tail == std::string::npos || inside[tail] != '/') return false; // Без завершающего слеша тег не самозакрывающийся.
    bool single_quote = false, double_quote = false, unquoted_value = false, after_equal = false; // Отслеживаем состояние разбора атрибутов.
    for (std::size_t i = name_end; i < tail; ++i) { const char ch = inside[i]; if (single_quote) { if (ch == '\'') single_quote = false; continue; } if (double_quote) { if (ch == '"') double_quote = false; continue; } if (ch == '\'' && after_equal) { single_quote = true; after_equal = false; unquoted_value = false; } else if (ch == '"' && after_equal) { double_quote = true; after_equal = false; unquoted_value = false; } else if (ch == '=') { after_equal = true; unquoted_value = false; } else if (std::isspace(static_cast<unsigned char>(ch))) { unquoted_value = false; } else if (after_equal) { after_equal = false; unquoted_value = true; } } // Анализируем кавычки, разделители и некавыченные значения.
    return !unquoted_value && !single_quote && !double_quote; // Слеш является синтаксическим, если он не завершает некавыченное значение.
} // Завершаем проверку самозакрывающегося тега.
}
void parse_html(const std::string& source, HtmlAnalysisResult& result) { // Выполняем последовательный ручной разбор.
    const std::vector<Position> positions = build_positions(source); // Рассчитываем координаты один раз для всего документа.
    std::size_t i = 0; // Начинаем с первого символа.
    while (i < source.size()) { // Обрабатываем остаток документа.
        const std::size_t start = i; // Запоминаем начало токена.
        const auto p = positions[start]; // Получаем координаты из таблицы.
        HtmlToken token; token.offset = start; token.line = p.first; token.column = p.second; // Инициализируем токен и его позицию.
        if (source.compare(i, 4, "<!--") == 0) { // Проверяем начало комментария.
            const std::size_t end = source.find("-->", i + 4); token.type = HtmlTokenType::Comment; // Ищем конец и назначаем тип комментария.
            if (end == std::string::npos) { token.raw_text = source.substr(i); diagnostic(result, "Лексическая ошибка", "HTML-комментарий не закрыт последовательностью -->.", positions, start); i = source.size(); } // Обрабатываем незакрытый комментарий.
            else { i = end + 3; token.raw_text = source.substr(start, i - start); } // Сохраняем корректный комментарий.
            ++result.comments; result.tokens.push_back(token); continue; // Учитываем комментарий и продолжаем разбор.
        }
        if (source.compare(i, 2, "<!") == 0 || source.compare(i, 2, "<?") == 0) { // Распознаём декларации и инструкции обработки.
            bool single_quote = false, double_quote = false; // Отслеживаем кавычки внутри декларации.
            std::size_t declaration_end = i + 2; // Начинаем поиск конца после вводного символа.
            for (; declaration_end < source.size(); ++declaration_end) { const char ch = source[declaration_end]; if (ch == '\'' && !double_quote) single_quote = !single_quote; else if (ch == '"' && !single_quote) double_quote = !double_quote; else if (ch == '>' && !single_quote && !double_quote) break; } // Ищем > вне кавычек.
            token.type = HtmlTokenType::Declaration; // Отмечаем декларацию отдельно от обычного тега.
            if (declaration_end >= source.size()) { token.raw_text = source.substr(i); diagnostic(result, "Лексическая ошибка", "HTML-декларация не завершена символом >.", positions, start); i = source.size(); } // Диагностируем незавершённую декларацию.
            else { i = declaration_end + 1; token.raw_text = source.substr(start, i - start); } // Сохраняем корректную декларацию.
            result.tokens.push_back(token); continue; // Продолжаем разбор следующего фрагмента.
        }
        if (source[i] != '<') { // Обрабатываем текст вне тегов.
            const std::size_t end = source.find('<', i); i = end == std::string::npos ? source.size() : end; // Ищем следующую разметку.
            token.type = HtmlTokenType::Text; token.raw_text = source.substr(start, i - start); ++result.text_segments; result.tokens.push_back(token); continue; // Сохраняем текстовый токен.
        }
        bool single = false, dual = false; std::size_t end = i + 1; // Подготавливаем поиск конца тега с учётом кавычек.
        for (; end < source.size(); ++end) { const char ch = source[end]; if (ch == '\'' && !dual) single = !single; else if (ch == '"' && !single) dual = !dual; else if (ch == '>' && !single && !dual) break; } // Ищем > вне кавычек.
        if (end == source.size()) { diagnostic(result, "Лексическая ошибка", "Тег не завершён символом >.", positions, start); token.type = HtmlTokenType::Text; token.raw_text = source.substr(start); result.tokens.push_back(token); break; } // Обрабатываем незавершённый тег.
        i = end + 1; token.raw_text = source.substr(start, i - start); const std::string inside = source.substr(start + 1, end - start - 1); // Сохраняем тег и выделяем его внутреннее содержимое.
        std::size_t cursor = 0; while (cursor < inside.size() && std::isspace(static_cast<unsigned char>(inside[cursor]))) ++cursor; // Пропускаем начальные пробелы.
        const bool closing = cursor < inside.size() && inside[cursor] == '/'; if (closing) ++cursor; // Определяем закрывающий тег.
        while (cursor < inside.size() && std::isspace(static_cast<unsigned char>(inside[cursor]))) ++cursor; const std::size_t name_start = cursor; // Находим начало имени.
        while (cursor < inside.size() && name_char(inside[cursor])) ++cursor; const std::size_t name_end = cursor; // Считываем имя и запоминаем границу имени.
        if (cursor == name_start) { diagnostic(result, "Лексическая ошибка", "После символа < не удалось определить имя тега.", positions, start); token.type = HtmlTokenType::Text; result.tokens.push_back(token); continue; } // Обрабатываем тег без имени.
        token.name = lower_ascii(inside.substr(name_start, cursor - name_start)); // Сохраняем имя тега без учёта регистра.
        if (closing) { // Проверяем синтаксис закрывающего тега.
            while (cursor < inside.size() && std::isspace(static_cast<unsigned char>(inside[cursor]))) ++cursor; // Разрешаем пробелы после имени.
            if (cursor != inside.size()) diagnostic(result, "Лексическая ошибка", "После имени закрывающего тега обнаружены лишние символы.", positions, start + 1 + cursor); // Сообщаем о лишнем содержимом.
            token.type = HtmlTokenType::CloseTag; ++result.closing_tags; // Учитываем закрывающий тег.
        }
        else { // Разбираем открывающий или самозакрывающийся тег.
            const std::size_t tail = inside.find_last_not_of(" \t\r\n"); const bool self = is_self_closing(inside, name_end, tail); const std::size_t attributes_end = self ? tail : inside.size(); token.type = self ? HtmlTokenType::SelfClosingTag : HtmlTokenType::OpenTag; // Определяем вид тега и границу атрибутов.
            if (self) ++result.self_closing_tags; else ++result.opening_tags; // Обновляем счётчик тегов.
            while (cursor < attributes_end) { // Последовательно читаем атрибуты.
                while (cursor < attributes_end && std::isspace(static_cast<unsigned char>(inside[cursor]))) ++cursor; if (cursor >= attributes_end) break; // Пропускаем пробелы и проверяем конец.
                const std::size_t astart = cursor; while (cursor < attributes_end && name_char(inside[cursor])) ++cursor; if (cursor == astart) { ++cursor; continue; } // Считываем имя атрибута и избегаем зацикливания.
                HtmlAttribute attr; attr.name = inside.substr(astart, cursor - astart); while (cursor < attributes_end && std::isspace(static_cast<unsigned char>(inside[cursor]))) ++cursor; // Создаём атрибут и пропускаем пробелы.
                if (cursor < attributes_end && inside[cursor] == '=') { // Проверяем наличие значения.
                    ++cursor; while (cursor < attributes_end && std::isspace(static_cast<unsigned char>(inside[cursor]))) ++cursor; // Пропускаем знак равенства и пробелы.
                    if (cursor < attributes_end && (inside[cursor] == '"' || inside[cursor] == '\'')) { const char quote = inside[cursor++]; const std::size_t vstart = cursor; while (cursor < attributes_end && inside[cursor] != quote) ++cursor; attr.value = inside.substr(vstart, cursor - vstart); if (cursor < attributes_end) ++cursor; else diagnostic(result, "Лексическая ошибка", "Значение атрибута не закрыто кавычкой.", positions, start); } // Читаем значение в кавычках.
                    else { const std::size_t vstart = cursor; while (cursor < attributes_end && !std::isspace(static_cast<unsigned char>(inside[cursor]))) ++cursor; attr.value = inside.substr(vstart, cursor - vstart); } // Сохраняем слеши внутри некавыченного значения.
                }
                token.attributes.push_back(attr); ++result.attributes; // Сохраняем атрибут и увеличиваем счётчик.
            }
        }
        result.tokens.push_back(token); // Добавляем токен в исходную последовательность.
    }
}
