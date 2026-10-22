#include "Header.h" // Подключаем общие объявления.

namespace { // Ограничиваем вспомогательные элементы текущим файлом.
const std::vector<std::string> void_elements = { "area", "base", "br", "col", "embed", "hr", "img", "input", "link", "meta", "param", "source", "track", "wbr" }; // Перечисляем элементы без закрывающего тега.
struct OpenElement { std::string name; std::size_t line; std::size_t column; }; // Храним имя и координаты открытого элемента.
bool is_void_element(const std::string& name) { return std::find(void_elements.begin(), void_elements.end(), name) != void_elements.end(); } // Проверяем void-элемент.
}

void validate_html_structure(HtmlAnalysisResult& result) { // Проверяем вложенность с восстановлением после ошибок.
    std::vector<OpenElement> stack; // Создаём стек открытых тегов.
    for (const HtmlToken& token : result.tokens) { // Обрабатываем токены по порядку.
        if (token.type == HtmlTokenType::OpenTag && !is_void_element(token.name)) { stack.push_back({ token.name, token.line, token.column }); continue; } // Добавляем обычный открывающий тег.
        if (token.type != HtmlTokenType::CloseTag) continue; // Остальные виды токенов не меняют стек.
        if (stack.empty()) { result.diagnostics.push_back({ "Структурная ошибка", "Закрывающий тег </" + token.name + "> не имеет открывающей пары.", token.line, token.column }); continue; } // Диагностируем закрывающий тег без открытых элементов.
        if (stack.back().name == token.name) { stack.pop_back(); continue; } // Закрываем верхний элемент при совпадении имени.
        auto matching = std::find_if(stack.rbegin(), stack.rend(), [&](const OpenElement& element) { return element.name == token.name; }); // Ищем соответствующий открывающий тег глубже в стеке.
        if (matching == stack.rend()) { result.diagnostics.push_back({ "Структурная ошибка", "Закрывающий тег </" + token.name + "> не имеет открывающей пары; ожидался закрывающий тег для <" + stack.back().name + ">.", token.line, token.column }); continue; } // Не меняем стек, если соответствующего открывающего тега нет.
        result.diagnostics.push_back({ "Структурная ошибка", "Нарушена вложенность: тег </" + token.name + "> встретился до закрытия элемента <" + stack.back().name + ">.", token.line, token.column }); // Фиксируем первичное нарушение порядка.
        while (!stack.empty() && stack.back().name != token.name) { const OpenElement& unclosed = stack.back(); result.diagnostics.push_back({ "Структурная ошибка", "Открывающий тег <" + unclosed.name + "> не закрыт до закрывающего тега </" + token.name + ">.", unclosed.line, unclosed.column }); stack.pop_back(); } // Закрываем ошибочно вложенные элементы, чтобы избежать каскадных ложных диагностик.
        if (!stack.empty()) stack.pop_back(); // Снимаем найденную пару после восстановления стека.
    }
    for (const OpenElement& element : stack) result.diagnostics.push_back({ "Структурная ошибка", "Открывающий тег <" + element.name + "> не имеет закрывающего тега.", element.line, element.column }); // Сообщаем об элементах, оставшихся открытыми.
}
