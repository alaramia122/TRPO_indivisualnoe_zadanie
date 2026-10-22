#include "Header.h" // Подключаем общие объявления.
namespace { // Ограничиваем вспомогательные элементы текущим файлом.
const std::vector<std::string> void_elements = { "area", "base", "br", "col", "embed", "hr", "img", "input", "link", "meta", "param", "source", "track", "wbr" }; // Перечисляем элементы без закрывающего тега.
struct OpenElement { std::string name; std::size_t line; std::size_t column; }; // Храним имя и координаты открытого элемента.
bool is_void_element(const std::string& name) { return std::find(void_elements.begin(), void_elements.end(), name) != void_elements.end(); } // Проверяем void-элемент.
}
void validate_html_structure(HtmlAnalysisResult& result) { // Проверяем вложенность с помощью стека.
    std::vector<OpenElement> stack; // Создаём стек открытых тегов.
    for (const HtmlToken& token : result.tokens) { // Обрабатываем токены по порядку.
        if (token.type == HtmlTokenType::OpenTag && !is_void_element(token.name)) stack.push_back({ token.name, token.line, token.column }); // Добавляем обычный открывающий тег.
        else if (token.type == HtmlTokenType::CloseTag) { // Обрабатываем закрывающий тег.
            if (stack.empty()) result.diagnostics.push_back({ "Структурная ошибка", "Закрывающий тег </" + token.name + "> не имеет открывающей пары.", token.line, token.column }); // Диагностируем лишний закрывающий тег.
            else if (stack.back().name == token.name) stack.pop_back(); // Закрываем верхний элемент при совпадении имени.
            else result.diagnostics.push_back({ "Структурная ошибка", "Тег </" + token.name + "> не соответствует ожидаемому закрывающему тегу для <" + stack.back().name + ">.", token.line, token.column }); // Диагностируем нарушение вложенности.
        } // Самозакрывающиеся теги, комментарии и текст не меняют стек.
    } // Завершаем обход токенов.
    for (const OpenElement& element : stack) result.diagnostics.push_back({ "Структурная ошибка", "Открывающий тег <" + element.name + "> не имеет закрывающего тега.", element.line, element.column }); // Сообщаем об элементах, оставшихся открытыми.
}
