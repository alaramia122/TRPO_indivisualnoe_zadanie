#include "Header.h" // Подключаем общие объявления.

namespace { // Ограничиваем вспомогательные элементы текущим файлом.
class PositionIndex { // Индексируем позиции за один проход по исходному тексту.
public:
    explicit PositionIndex(const std::string& source) : codepoint_prefix_(source.size() + 1, 0) { // Подготавливаем префиксные суммы и начала строк.
        line_starts_.push_back(0); // Первая строка начинается с нулевого смещения.
        for (std::size_t i = 0; i < source.size(); ++i) { // Проходим по исходным байтам один раз.
            const unsigned char ch = static_cast<unsigned char>(source[i]); // Читаем байт без знакового расширения.
            const bool continuation = (ch & 0xC0) == 0x80; // Определяем продолжение UTF-8 последовательности.
            const bool newline_byte = ch == '\r' || ch == '\n'; // Исключаем символы перевода строки из номера столбца.
            codepoint_prefix_[i + 1] = codepoint_prefix_[i] + ((!continuation && !newline_byte) ? 1u : 0u); // Считаем Unicode-кодовые точки, а не байты UTF-8.
            if (ch == '\r') { // Обрабатываем CR и CRLF как один перевод строки.
                if (i + 1 < source.size() && source[i + 1] == '\n') line_starts_.push_back(i + 2); // Для CRLF следующая строка начинается после LF.
                else line_starts_.push_back(i + 1); // Для одиночного CR следующая строка начинается сразу после него.
            } else if (ch == '\n' && (i == 0 || source[i - 1] != '\r')) line_starts_.push_back(i + 1); // Обрабатываем одиночный LF.
        }
    }
    std::pair<std::size_t, std::size_t> at(std::size_t offset) const { // Возвращаем координаты для смещения.
        if (offset >= codepoint_prefix_.size()) offset = codepoint_prefix_.size() - 1; // Ограничиваем смещение размером исходного текста.
        const auto it = std::upper_bound(line_starts_.begin(), line_starts_.end(), offset); // Находим начало строки двоичным поиском.
        const std::size_t line_index = static_cast<std::size_t>(it - line_starts_.begin() - 1); // Вычисляем индекс строки.
        const std::size_t line_start = line_starts_[line_index]; // Получаем смещение начала строки.
        return { line_index + 1, codepoint_prefix_[offset] - codepoint_prefix_[line_start] + 1 }; // Возвращаем строку и столбец с единицы.
    }
private:
    std::vector<std::size_t> line_starts_; // Храним смещения начал строк.
    std::vector<std::size_t> codepoint_prefix_; // Храним число UTF-8 кодовых точек до каждого смещения.
};
std::string lower_ascii(std::string value) { // Нормализуем имя тега.
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); }); // Переводим латиницу в нижний регистр.
    return value; // Возвращаем нормализованную строку.
}
bool name_char(char ch) { const unsigned char c = static_cast<unsigned char>(ch); return std::isalnum(c) || ch == '-' || ch == '_' || ch == ':' || c >= 128; } // Проверяем символ имени тега или атрибута.
void diagnostic(HtmlAnalysisResult& result, const std::string& category, const std::string& message, const PositionIndex& positions, std::size_t offset) { const auto p = positions.at(offset); result.diagnostics.push_back({ category, message, p.first, p.second }); } // Добавляем сообщение с координатами.
bool is_self_closing(const std::string& inside, std::size_t cursor) { // Определяем синтаксический слеш, не путая его со слешем в значении атрибута.
    if (inside.empty() || inside.back() != '/') return false; // Самозакрывающийся маркер должен непосредственно предшествовать >.
    while (cursor < inside.size()) { // Просматриваем остаток открывающего тега.
        while (cursor < inside.size() && std::isspace(static_cast<unsigned char>(inside[cursor]))) ++cursor; // Пропускаем пробелы между атрибутами.
        if (cursor == inside.size() - 1 && inside[cursor] == '/') return true; // Слеш вне значения атрибута является маркером самозакрытия.
        const std::size_t name_start = cursor; // Запоминаем начало имени атрибута.
        while (cursor < inside.size() && name_char(inside[cursor])) ++cursor; // Пропускаем имя атрибута.
        if (cursor == name_start) { ++cursor; continue; } // Пропускаем неожиданный символ, не допуская зацикливания.
        while (cursor < inside.size() && std::isspace(static_cast<unsigned char>(inside[cursor]))) ++cursor; // Пропускаем пробелы перед знаком равенства.
        if (cursor >= inside.size() || inside[cursor] != '=') continue; // Атрибут без значения не влияет на распознавание слеша.
        ++cursor; // Пропускаем знак равенства.
        while (cursor < inside.size() && std::isspace(static_cast<unsigned char>(inside[cursor]))) ++cursor; // Пропускаем пробелы перед значением.
        if (cursor < inside.size() && (inside[cursor] == '"' || inside[cursor] == '\'')) { // Пропускаем значение в кавычках целиком.
            const char quote = inside[cursor++]; // Запоминаем ограничивающую кавычку.
            while (cursor < inside.size() && inside[cursor] != quote) ++cursor; // Ищем закрывающую кавычку.
            if (cursor < inside.size()) ++cursor; // Переходим за закрывающую кавычку, если она найдена.
        } else { // Обрабатываем некавыченное значение.
            const std::size_t value_start = cursor; // Запоминаем начало значения.
            while (cursor < inside.size() && !std::isspace(static_cast<unsigned char>(inside[cursor]))) ++cursor; // Слеш без пробела остаётся частью значения.
            if (cursor == inside.size() && value_start < inside.size()) return false; // Если значение дошло до завершающего слеша, это не маркер самозакрытия.
        }
    }
    return true; // Завершающий слеш находится вне значения атрибута.
}
}

void parse_html(const std::string& source, HtmlAnalysisResult& result) { // Выполняем последовательный ручной разбор.
    const PositionIndex positions(source); // Один раз строим индекс координат всего документа.
    std::size_t i = 0; // Начинаем с первого байта.
    while (i < source.size()) { // Обрабатываем остаток документа.
        const std::size_t start = i; // Запоминаем начало токена.
        const auto p = positions.at(start); // Получаем координаты начала токена.
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
            token.type = HtmlTokenType::Declaration; // Отмечаем декларацию отдельным типом токена.
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
        i = end + 1; token.raw_text = source.substr(start, i - start); const std::string inside = source.substr(start + 1, end - start - 1); // Сохраняем исходный тег и выделяем содержимое.
        std::size_t cursor = 0; while (cursor < inside.size() && std::isspace(static_cast<unsigned char>(inside[cursor]))) ++cursor; // Пропускаем начальные пробелы.
        const bool closing = cursor < inside.size() && inside[cursor] == '/'; if (closing) ++cursor; // Определяем закрывающий тег.
        while (cursor < inside.size() && std::isspace(static_cast<unsigned char>(inside[cursor]))) ++cursor; const std::size_t name_start = cursor; // Находим начало имени.
        while (cursor < inside.size() && name_char(inside[cursor])) ++cursor; // Считываем имя тега.
        if (cursor == name_start) { diagnostic(result, "Лексическая ошибка", "После символа < не удалось определить имя тега.", positions, start); token.type = HtmlTokenType::Text; result.tokens.push_back(token); continue; } // Обрабатываем тег без имени.
        token.name = lower_ascii(inside.substr(name_start, cursor - name_start)); // Сохраняем имя тега без учёта регистра.
        if (closing) { // Проверяем остаток закрывающего тега до изменения счётчиков.
            std::size_t tail = cursor; while (tail < inside.size() && std::isspace(static_cast<unsigned char>(inside[tail]))) ++tail; // Допускаем пробелы после имени.
            if (tail != inside.size()) { diagnostic(result, "Лексическая ошибка", "После имени закрывающего тега обнаружены лишние символы.", positions, start); token.type = HtmlTokenType::Text; result.tokens.push_back(token); continue; } // Не используем некорректный тег для проверки вложенности.
            token.type = HtmlTokenType::CloseTag; ++result.closing_tags; // Учитываем корректный закрывающий тег.
        } else { // Разбираем открывающий или самозакрывающийся тег.
            const bool self = is_self_closing(inside, cursor); token.type = self ? HtmlTokenType::SelfClosingTag : HtmlTokenType::OpenTag; // Распознаём маркер самозакрытия вне некавыченных значений.
            if (self) ++result.self_closing_tags; else ++result.opening_tags; // Обновляем счётчик открывающих тегов.
            while (cursor < inside.size()) { // Последовательно читаем атрибуты.
                while (cursor < inside.size() && (std::isspace(static_cast<unsigned char>(inside[cursor])) || inside[cursor] == '/')) ++cursor; if (cursor >= inside.size()) break; // Пропускаем разделители.
                const std::size_t astart = cursor; while (cursor < inside.size() && name_char(inside[cursor])) ++cursor; if (cursor == astart) { ++cursor; continue; } // Считываем имя атрибута и избегаем зацикливания.
                HtmlAttribute attr; attr.name = inside.substr(astart, cursor - astart); while (cursor < inside.size() && std::isspace(static_cast<unsigned char>(inside[cursor]))) ++cursor; // Создаём атрибут и пропускаем пробелы.
                if (cursor < inside.size() && inside[cursor] == '=') { // Проверяем наличие значения.
                    ++cursor; while (cursor < inside.size() && std::isspace(static_cast<unsigned char>(inside[cursor]))) ++cursor; // Пропускаем знак равенства и пробелы.
                    if (cursor < inside.size() && (inside[cursor] == '"' || inside[cursor] == '\'')) { const char quote = inside[cursor++]; const std::size_t vstart = cursor; while (cursor < inside.size() && inside[cursor] != quote) ++cursor; attr.value = inside.substr(vstart, cursor - vstart); if (cursor < inside.size()) ++cursor; else diagnostic(result, "Лексическая ошибка", "Значение атрибута не закрыто кавычкой.", positions, start); } // Читаем значение в кавычках.
                    else { const std::size_t vstart = cursor; while (cursor < inside.size() && !std::isspace(static_cast<unsigned char>(inside[cursor]))) ++cursor; attr.value = inside.substr(vstart, cursor - vstart); } // Слеш внутри некавыченного значения является частью значения.
                }
                token.attributes.push_back(attr); ++result.attributes; // Сохраняем атрибут и увеличиваем счётчик.
            }
        }
        result.tokens.push_back(token); // Добавляем токен в исходную последовательность.
    }
}
