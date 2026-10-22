// Подключаем общие объявления.
#include "Header.h"

// Ограничиваем вспомогательные элементы текущим файлом.
namespace {
// Индексируем позиции за один проход по исходному тексту.
class PositionIndex {
public:
    // Подготавливаем префиксные суммы и начала строк.
    explicit PositionIndex(const std::string& source) : codepoint_prefix_(source.size() + 1,
         0) {
        // Первая строка начинается с нулевого смещения.
        line_starts_.push_back(0);

        // Проходим по исходным байтам один раз.
        for (std::size_t i = 0; i < source.size(); ++i) {
            // Читаем байт через unsigned char для сохранения диапазона значений.
            const unsigned char ch = static_cast<unsigned char>(source[i]);

            // Определяем продолжение UTF-8 последовательности.
            const bool continuation = (ch & 0xC0) == 0x80;

            // Отделяем символы перевода строки от подсчёта столбца.
            const bool newline_byte = ch == '\r' || ch == '\n';

            // Количество столбцов определяем по Unicode-кодовым точкам UTF-8.
            codepoint_prefix_[i + 1] = codepoint_prefix_[i] + ((!continuation && !newline_byte) ? 1u : 0u);

            // Обрабатываем CR и CRLF как один перевод строки.
            if (ch == '\r') {
                // Для CRLF следующая строка начинается после LF.
                if (i + 1 < source.size() && source[i + 1] == '\n') line_starts_.push_back(i + 2);

                // Для одиночного CR следующая строка начинается сразу после него.
                else line_starts_.push_back(i + 1);

            // Обрабатываем одиночный LF.
            } else if (ch == '\n' && (i == 0 || source[i - 1] != '\r')) line_starts_.push_back(i + 1);

        }
    }
    // Возвращаем координаты для смещения.
    std::pair<std::size_t,
         std::size_t> at(std::size_t offset) const {
        // Ограничиваем смещение размером исходного текста.
        if (offset >= codepoint_prefix_.size()) offset = codepoint_prefix_.size() - 1;

        // Находим начало строки двоичным поиском.
        const auto it = std::upper_bound(line_starts_.begin(),
             line_starts_.end(),
             offset);

        // Вычисляем индекс строки.
        const std::size_t line_index = static_cast<std::size_t>(it - line_starts_.begin() - 1);

        // Получаем смещение начала строки.
        const std::size_t line_start = line_starts_[line_index];

        // Возвращаем строку и столбец с единицы.
        return { line_index + 1,
             codepoint_prefix_[offset] - codepoint_prefix_[line_start] + 1 };

    }
private:
    // Храним смещения начал строк.
    std::vector<std::size_t> line_starts_;

    // Храним число UTF-8 кодовых точек до каждого смещения.
    std::vector<std::size_t> codepoint_prefix_;

};

// Нормализуем имя тега.
std::string lower_ascii(std::string value) {
    // Переводим латиницу в нижний регистр.
    std::transform(value.begin(),
         value.end(),
         value.begin(),
         [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });

    // Возвращаем нормализованную строку.
    return value;

}
// Проверяем символ имени тега или атрибута.
bool name_char(char ch) { const unsigned char c = static_cast<unsigned char>(ch);
     return std::isalnum(c) || ch == '-' || ch == '_' || ch == ':' || c >= 128;
     }
// Добавляем сообщение с координатами.
void diagnostic(HtmlAnalysisResult& result,
     const std::string& category,
     const std::string& message,
     const PositionIndex& positions,
     std::size_t offset) { const auto p = positions.at(offset);
     result.diagnostics.push_back({ category,
     message,
     p.first,
     p.second });
     }
// Отличаем синтаксический слеш от слеша внутри значения атрибута.
bool is_self_closing(const std::string& inside,
     std::size_t cursor) {
    // Самозакрывающийся маркер должен непосредственно предшествовать >.
    if (inside.empty() || inside.back() != '/') return false;

    // Просматриваем остаток открывающего тега.
    while (cursor < inside.size()) {
        // Пропускаем пробелы между атрибутами.
        while (cursor < inside.size() &&
               std::isspace(
                   static_cast<unsigned char>(inside[cursor]))) {
            ++cursor;
        }

        // Слеш вне значения атрибута является маркером самозакрытия.
        if (cursor == inside.size() - 1 && inside[cursor] == '/') return true;

        // Запоминаем начало имени атрибута.
        const std::size_t name_start = cursor;

        // Пропускаем имя атрибута.
        while (cursor < inside.size() && name_char(inside[cursor])) ++cursor;

        // Пропускаем неожиданный символ и продвигаем курсор для продолжения разбора.
        if (cursor == name_start) { ++cursor;
             continue;
             }
        // Пропускаем пробелы перед знаком равенства.
        while (cursor < inside.size() &&
               std::isspace(
                   static_cast<unsigned char>(inside[cursor]))) {
            ++cursor;
        }

        // Распознавание слеша учитывает символы вне значения атрибута.
        if (cursor >= inside.size() || inside[cursor] != '=') continue;

        // Пропускаем знак равенства.
        ++cursor;

        // Пропускаем пробелы перед значением.
        while (cursor < inside.size() &&
               std::isspace(
                   static_cast<unsigned char>(inside[cursor]))) {
            ++cursor;
        }

        // Пропускаем значение в кавычках целиком.
        if (cursor < inside.size() && (inside[cursor] == '"' || inside[cursor] == '\'')) {
            // Запоминаем ограничивающую кавычку.
            const char quote = inside[cursor++];

            // Ищем закрывающую кавычку.
            while (cursor < inside.size() && inside[cursor] != quote) ++cursor;

            // Переходим за закрывающую кавычку, если она найдена.
            if (cursor < inside.size()) ++cursor;

        // Обрабатываем значение в свободной записи.
        } else {
            // Запоминаем начало значения.
            const std::size_t value_start = cursor;

            // Слеш, примыкающий к значению, остаётся частью значения.
            while (cursor < inside.size() &&
                   !std::isspace(
                       static_cast<unsigned char>(inside[cursor]))) {
                ++cursor;
            }

            // Если значение заканчивается слешем, этот символ относится к значению атрибута.
            if (cursor == inside.size() && value_start < inside.size()) return false;

        }
    }
    // Завершающий слеш находится вне значения атрибута.
    return true;

}
}

// Выполняем последовательный ручной разбор.
void parse_html(const std::string& source,
     HtmlAnalysisResult& result) {
    // Один раз строим индекс координат всего документа.
    const PositionIndex positions(source);

    // Начинаем с первого байта.
    std::size_t i = 0;

    // Обрабатываем остаток документа.
    while (i < source.size()) {
        // Запоминаем начало токена.
        const std::size_t start = i;

        // Получаем координаты начала токена.
        const auto p = positions.at(start);

        // Инициализируем токен и его позицию.
        HtmlToken token;
             token.offset = start;
             token.line = p.first;
             token.column = p.second;

        // Проверяем начало комментария.
        if (source.compare(i,
             4,
             "<!--") == 0) {
            // Ищем конец и назначаем тип комментария.
            const std::size_t end = source.find("-->",
                 i + 4);
                 token.type = HtmlTokenType::Comment;

            // Обрабатываем комментарий, продолжающийся до конца файла.
            if (end == std::string::npos) { token.raw_text = source.substr(i);
                 diagnostic(result,
                 "Лексическая ошибка",
                 "Для HTML-комментария ожидается завершающая последовательность -->.",
                 positions,
                 start);
                 i = source.size();
                 }
            // Сохраняем корректный комментарий.
            else { i = end + 3;
                 token.raw_text = source.substr(start,
                 i - start);
                 }
            // Учитываем комментарий и продолжаем разбор.
            ++result.comments;
                 result.tokens.push_back(token);
                 continue;

        }
        // Распознаём декларации и инструкции обработки.
        if (source.compare(i,
             2,
             "<!") == 0 || source.compare(i,
             2,
             "<?") == 0) {
            // Отслеживаем кавычки внутри декларации.
            bool single_quote = false,
                 double_quote = false;

            // Начинаем поиск конца после вводного символа.
            std::size_t declaration_end = i + 2;

            // Ищем > вне кавычек.
            for (; declaration_end < source.size(); ++declaration_end) { const char ch = source[declaration_end];
                 // Кавычки одного типа переключают состояние только вне кавычек другого типа.
                 if (ch == '\'' && !double_quote) single_quote = !single_quote;
                 else if (ch == '"' && !single_quote) double_quote = !double_quote;
                 else if (ch == '>' && !single_quote && !double_quote) break;
                 }
            // Отмечаем декларацию отдельным типом токена.
            token.type = HtmlTokenType::Declaration;

            // Диагностируем декларацию, продолжающуюся до конца файла.
            if (declaration_end >= source.size()) { token.raw_text = source.substr(i);
                 diagnostic(result,
                 "Лексическая ошибка",
                 "Для HTML-декларации ожидается завершающий символ >.",
                 positions,
                 start);
                 i = source.size();
                 }
            // Сохраняем корректную декларацию.
            else { i = declaration_end + 1;
                 token.raw_text = source.substr(start,
                 i - start);
                 }
            // Продолжаем разбор следующего фрагмента.
            result.tokens.push_back(token);
                 continue;

        }
        // Обрабатываем текст вне тегов.
        if (source[i] != '<') {
            // Ищем следующую разметку.
            const std::size_t end = source.find('<',
                 i);
                 i = end == std::string::npos ? source.size() : end;

            // Сохраняем текстовый токен.
            token.type = HtmlTokenType::Text;
                 token.raw_text = source.substr(start,
                 i - start);
                 ++result.text_segments;
                 result.tokens.push_back(token);
                 continue;

        }
        // Подготавливаем поиск конца тега с учётом кавычек.
        bool single = false,
             dual = false;
             std::size_t end = i + 1;

        // Ищем > вне кавычек.
        // Символ > внутри атрибута в кавычках остаётся частью значения.
        for (; end < source.size(); ++end) { const char ch = source[end];
             if (ch == '\'' && !dual) single = !single;
             else if (ch == '"' && !single) dual = !dual;
             else if (ch == '>' && !single && !dual) break;
             }
        // Обрабатываем тег, продолжающийся до конца файла.
        if (end == source.size()) { diagnostic(result,
             "Лексическая ошибка",
             "Для тега ожидается завершающий символ >.",
             positions,
             start);
             token.type = HtmlTokenType::Text;
             token.raw_text = source.substr(start);
             result.tokens.push_back(token);
             break;
             }
        // Сохраняем исходный тег и выделяем содержимое.
        i = end + 1;
             token.raw_text = source.substr(start,
             i - start);
             const std::string inside = source.substr(start + 1,
             end - start - 1);

        // Пропускаем начальные пробелы.
        std::size_t cursor = 0;
             while (cursor < inside.size() &&
                    std::isspace(
                        static_cast<unsigned char>(inside[cursor]))) {
                 ++cursor;
             }

        // Определяем закрывающий тег.
        const bool closing = cursor < inside.size() && inside[cursor] == '/';
             if (closing) ++cursor;

        // Находим начало имени.
        while (cursor < inside.size() &&
               std::isspace(
                   static_cast<unsigned char>(inside[cursor]))) {
            ++cursor;
        }
             const std::size_t name_start = cursor;

        // Считываем имя тега.
        while (cursor < inside.size() && name_char(inside[cursor])) ++cursor;

        // Обрабатываем конструкцию с пустой позицией имени тега.
        if (cursor == name_start) { diagnostic(result,
             "Лексическая ошибка",
             "После символа < ожидается имя тега.",
             positions,
             start);
             token.type = HtmlTokenType::Text;
             result.tokens.push_back(token);
             continue;
             }
        // Сохраняем имя тега в нижнем регистре.
        token.name = lower_ascii(inside.substr(name_start,
             cursor - name_start));

        // Проверяем остаток закрывающего тега до изменения счётчиков.
        if (closing) {
            // Допускаем пробелы после имени.
            std::size_t tail = cursor;
                 while (tail < inside.size() && std::isspace(static_cast<unsigned char>(inside[tail]))) ++tail;

            // Передаём в проверку вложенности теги с корректной завершающей частью.
            if (tail != inside.size()) { diagnostic(result,
                 "Лексическая ошибка",
                 "После имени закрывающего тега обнаружены лишние символы.",
                 positions,
                 start);
                 token.type = HtmlTokenType::Text;
                 result.tokens.push_back(token);
                 continue;
                 }
            // Учитываем корректный закрывающий тег.
            token.type = HtmlTokenType::CloseTag;
                 ++result.closing_tags;

        // Разбираем открывающий или самозакрывающийся тег.
        } else {
            // Распознаём маркер самозакрытия за пределами значений атрибутов.
            const bool self = is_self_closing(inside,
                 cursor);
                 token.type = self ? HtmlTokenType::SelfClosingTag : HtmlTokenType::OpenTag;

            // Обновляем счётчик открывающих тегов.
            if (self) ++result.self_closing_tags;
                 else ++result.opening_tags;

            // Последовательно читаем атрибуты.
            while (cursor < inside.size()) {
                // Пропускаем разделители.
                while (cursor < inside.size() &&
                       (std::isspace(static_cast<unsigned char>(inside[cursor])) ||
                        inside[cursor] == '/')) {
                    ++cursor;
                }
                     // После пропуска пробелов и слешей проверяем, остались ли данные для атрибута.
                     if (cursor >= inside.size()) break;

                // Считываем имя атрибута и избегаем зацикливания.
                const std::size_t astart = cursor;
                     while (cursor < inside.size() && name_char(inside[cursor])) ++cursor;
                     if (cursor == astart) { ++cursor;
                     continue;
                     }
                // Создаём атрибут и пропускаем пробелы.
                HtmlAttribute attr;
                     attr.name = inside.substr(astart,
                     cursor - astart);
                     while (cursor < inside.size() &&
                            std::isspace(
                                static_cast<unsigned char>(inside[cursor]))) {
                         ++cursor;
                     }

                // Проверяем наличие значения.
                if (cursor < inside.size() && inside[cursor] == '=') {
                    // Пропускаем знак равенства и пробелы.
                    ++cursor;
                         while (cursor < inside.size() &&
                                std::isspace(
                                    static_cast<unsigned char>(inside[cursor]))) {
                             ++cursor;
                         }

                    // Читаем значение в кавычках.
                    if (cursor < inside.size() &&
                        (inside[cursor] == '"' || inside[cursor] == '\'')) {
                        const char quote = inside[cursor++];
                         const std::size_t vstart = cursor;
                         while (cursor < inside.size() && inside[cursor] != quote) ++cursor;
                         attr.value = inside.substr(vstart,
                         cursor - vstart);
                         if (cursor < inside.size()) ++cursor;
                         else diagnostic(result,
                         "Лексическая ошибка",
                         "Для значения атрибута ожидается закрывающая кавычка.",
                         positions,
                         start);
                         }
                    // Слеш внутри значения в свободной записи является частью значения.
                    else { const std::size_t vstart = cursor;
                         while (cursor < inside.size() &&
                                !std::isspace(
                                    static_cast<unsigned char>(inside[cursor]))) {
                             ++cursor;
                         }
                         attr.value = inside.substr(vstart,
                         cursor - vstart);
                         }
                }
                // Сохраняем атрибут и увеличиваем счётчик.
                token.attributes.push_back(attr);
                     ++result.attributes;

            }
        }
        // Добавляем токен в исходную последовательность.
        result.tokens.push_back(token);

    }
}
