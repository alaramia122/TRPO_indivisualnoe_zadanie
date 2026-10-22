#include "Header.h" // Подключаем общие объявления.
namespace { // Ограничиваем вспомогательные функции текущим файлом.
std::pair<std::size_t, std::size_t> position_at(const std::string& source, std::size_t offset) { // Вычисляем строку и столбец.
    std::size_t line = 1, column = 1; // Начинаем координаты с единицы.
    for (std::size_t i = 0; i < offset && i < source.size(); ++i) { if (source[i] == '\n') { ++line; column = 1; } else ++column; } // Считаем переводы строк и символы.
    return { line, column }; // Возвращаем координаты.
} // Завершаем вычисление позиции.
std::string lower_ascii(std::string value) { // Нормализуем имя тега.
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); }); // Переводим латиницу в нижний регистр.
    return value; // Возвращаем нормализованную строку.
} // Завершаем нормализацию.
bool name_char(char ch) { const unsigned char c = static_cast<unsigned char>(ch); return std::isalnum(c) || ch == '-' || ch == '_' || ch == ':' || c >= 128; } // Проверяем символ имени тега или атрибута.
void diagnostic(HtmlAnalysisResult& result, const std::string& category, const std::string& message, const std::string& source, std::size_t offset) { const auto p = position_at(source, offset); result.diagnostics.push_back({ category, message, p.first, p.second }); } // Добавляем сообщение с координатами.
} // Завершаем вспомогательные функции.
void parse_html(const std::string& source, HtmlAnalysisResult& result) { // Выполняем последовательный ручной разбор.
    std::size_t i = 0; // Начинаем с первого символа.
    while (i < source.size()) { // Обрабатываем остаток документа.
        const std::size_t start = i; // Запоминаем начало токена.
        const auto p = position_at(source, start); // Получаем координаты начала.
        HtmlToken token; token.offset = start; token.line = p.first; token.column = p.second; // Инициализируем токен и его позицию.
        if (source.compare(i, 4, "<!--") == 0) { // Проверяем начало комментария.
            const std::size_t end = source.find("-->", i + 4); token.type = HtmlTokenType::Comment; // Ищем конец и назначаем тип комментария.
            if (end == std::string::npos) { token.raw_text = source.substr(i); diagnostic(result, "Лексическая ошибка", "HTML-комментарий не закрыт последовательностью -->.", source, start); i = source.size(); } // Обрабатываем незакрытый комментарий.
            else { i = end + 3; token.raw_text = source.substr(start, i - start); } // Сохраняем корректный комментарий.
            ++result.comments; result.tokens.push_back(token); continue; // Учитываем комментарий и продолжаем разбор.
        } // Завершаем ветку комментариев.
        if (source.compare(i, 2, "<!") == 0 || source.compare(i, 2, "<?") == 0) { // Распознаём декларации, включая <!DOCTYPE html>, и инструкции обработки.
            bool single_quote = false, double_quote = false; // Отслеживаем кавычки внутри декларации.
            std::size_t declaration_end = i + 2; // Начинаем поиск конца после вводного символа.
            for (; declaration_end < source.size(); ++declaration_end) { const char ch = source[declaration_end]; if (ch == '\'' && !double_quote) single_quote = !single_quote; else if (ch == '"' && !single_quote) double_quote = !double_quote; else if (ch == '>' && !single_quote && !double_quote) break; } // Ищем > вне кавычек.
            token.type = HtmlTokenType::Declaration; // Отмечаем фрагмент как декларацию, а не как обычный тег или комментарий.
            if (declaration_end >= source.size()) { token.raw_text = source.substr(i); diagnostic(result, "Лексическая ошибка", "HTML-декларация не завершена символом >.", source, start); i = source.size(); } // Диагностируем незавершённую декларацию.
            else { i = declaration_end + 1; token.raw_text = source.substr(start, i - start); } // Сохраняем корректную декларацию и переходим за неё.
            result.tokens.push_back(token); // Добавляем декларацию в последовательность токенов.
            continue; // Продолжаем разбор следующего фрагмента.
        } // Завершаем обработку декларации.
        if (source[i] != '<') { // Обрабатываем текст вне тегов.
            const std::size_t end = source.find('<', i); i = end == std::string::npos ? source.size() : end; // Ищем следующую разметку.
            token.type = HtmlTokenType::Text; token.raw_text = source.substr(start, i - start); ++result.text_segments; result.tokens.push_back(token); continue; // Сохраняем текстовый токен.
        } // Завершаем обработку текста.
        bool single = false, dual = false; std::size_t end = i + 1; // Подготавливаем поиск конца тега с учётом кавычек.
        for (; end < source.size(); ++end) { const char ch = source[end]; if (ch == '\'' && !dual) single = !single; else if (ch == '"' && !single) dual = !dual; else if (ch == '>' && !single && !dual) break; } // Ищем > вне кавычек.
        if (end == source.size()) { diagnostic(result, "Лексическая ошибка", "Тег не завершён символом >.", source, start); token.type = HtmlTokenType::Text; token.raw_text = source.substr(start); result.tokens.push_back(token); break; } // Обрабатываем незавершённый тег.
        i = end + 1; token.raw_text = source.substr(start, i - start); const std::string inside = source.substr(start + 1, end - start - 1); // Сохраняем тег и выделяем его внутреннее содержимое.
        std::size_t cursor = 0; while (cursor < inside.size() && std::isspace(static_cast<unsigned char>(inside[cursor]))) ++cursor; // Пропускаем начальные пробелы.
        const bool closing = cursor < inside.size() && inside[cursor] == '/'; if (closing) ++cursor; // Определяем и пропускаем слеш закрывающего тега.
        while (cursor < inside.size() && std::isspace(static_cast<unsigned char>(inside[cursor]))) ++cursor; const std::size_t name_start = cursor; // Находим начало имени.
        while (cursor < inside.size() && name_char(inside[cursor])) ++cursor; // Считываем имя.
        if (cursor == name_start) { diagnostic(result, "Лексическая ошибка", "После символа < не удалось определить имя тега.", source, start); token.type = HtmlTokenType::Text; result.tokens.push_back(token); continue; } // Обрабатываем тег без имени.
        token.name = lower_ascii(inside.substr(name_start, cursor - name_start)); // Сохраняем имя тега без учёта регистра.
        if (closing) { token.type = HtmlTokenType::CloseTag; ++result.closing_tags; } // Учитываем закрывающий тег.
        else { // Разбираем открывающий или самозакрывающийся тег.
            const std::size_t tail = inside.find_last_not_of(" \t\r\n"); const bool self = tail != std::string::npos && inside[tail] == '/'; token.type = self ? HtmlTokenType::SelfClosingTag : HtmlTokenType::OpenTag; // Определяем вид открывающего тега.
            if (self) ++result.self_closing_tags; else ++result.opening_tags; // Обновляем счётчик тегов.
            while (cursor < inside.size()) { // Последовательно читаем атрибуты.
                while (cursor < inside.size() && (std::isspace(static_cast<unsigned char>(inside[cursor])) || inside[cursor] == '/')) ++cursor; if (cursor >= inside.size()) break; // Пропускаем разделители и проверяем конец.
                const std::size_t astart = cursor; while (cursor < inside.size() && name_char(inside[cursor])) ++cursor; if (cursor == astart) { ++cursor; continue; } // Считываем имя атрибута и избегаем зацикливания.
                HtmlAttribute attr; attr.name = inside.substr(astart, cursor - astart); while (cursor < inside.size() && std::isspace(static_cast<unsigned char>(inside[cursor]))) ++cursor; // Создаём атрибут и пропускаем пробелы.
                if (cursor < inside.size() && inside[cursor] == '=') { // Проверяем наличие значения.
                    ++cursor; while (cursor < inside.size() && std::isspace(static_cast<unsigned char>(inside[cursor]))) ++cursor; // Пропускаем знак равенства и пробелы.
                    if (cursor < inside.size() && (inside[cursor] == '"' || inside[cursor] == '\'')) { const char quote = inside[cursor++]; const std::size_t vstart = cursor; while (cursor < inside.size() && inside[cursor] != quote) ++cursor; attr.value = inside.substr(vstart, cursor - vstart); if (cursor < inside.size()) ++cursor; else diagnostic(result, "Лексическая ошибка", "Значение атрибута не закрыто кавычкой.", source, start); } // Читаем значение в кавычках.
                    else { const std::size_t vstart = cursor; while (cursor < inside.size() && !std::isspace(static_cast<unsigned char>(inside[cursor])) && inside[cursor] != '/') ++cursor; attr.value = inside.substr(vstart, cursor - vstart); } // Читаем некавыченное значение.
                } // Завершаем чтение значения.
                token.attributes.push_back(attr); ++result.attributes; // Сохраняем атрибут и увеличиваем счётчик.
            } // Завершаем разбор атрибутов.
        } // Завершаем обработку открывающего тега.
        result.tokens.push_back(token); // Добавляем токен в исходную последовательность.
    } // Завершаем сканирование документа.
} // Завершаем парсер.
