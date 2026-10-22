#include "Header.h"

namespace {

// Этим HTML-элементам по правилам рассматриваемого подмножества не нужна закрывающая пара.
const std::vector<std::string> void_elements = {
    "area", "base", "br", "col", "embed", "hr", "img",
    "input", "link", "meta", "param", "source", "track", "wbr"
};

// Координаты открытия сохраняются, чтобы указать точное место незакрытого элемента.
struct OpenElement {
    std::string name;
    std::size_t line;
    std::size_t column;
};

// Отделяем проверку списка void-элементов от основного алгоритма стека.
bool is_void_element(const std::string& name) {
    return std::find(
        void_elements.begin(),
        void_elements.end(),
        name) != void_elements.end();
}

}

// Проверяем вложенность парных тегов после лексического разбора исходного документа.
void validate_html_structure(HtmlAnalysisResult& result) {
    // Вершина стека всегда соответствует последнему ещё не закрытому элементу.
    std::vector<OpenElement> stack;

    // Последовательный проход сохраняет порядок исходных тегов и диагностик.
    for (const HtmlToken& token : result.tokens) {
        // Открывающий тег добавляется в стек, кроме элементов без закрывающей пары.
        if (token.type == HtmlTokenType::OpenTag &&
            !is_void_element(token.name)) {
            stack.push_back({ token.name, token.line, token.column });
            continue;
        }

        // Токены других типов не меняют состояние стека открытых элементов.
        if (token.type != HtmlTokenType::CloseTag) {
            continue;
        }

        // При пустом стеке текущий закрывающий тег не может соответствовать открытию.
        if (stack.empty()) {
            result.diagnostics.push_back({
                "Структурная ошибка",
                "Закрывающий тег </" + token.name +
                    "> не имеет открывающей пары.",
                token.line,
                token.column
            });
            continue;
        }

        // Если совпала вершина, вложенность корректна и достаточно снять один элемент.
        if (stack.back().name == token.name) {
            // После регистрации ошибки элемент больше не участвует в дальнейшей проверке.
            stack.pop_back();
            continue;
        }

        // Ищем соответствующий открывающий тег глубже в стеке.
        // Иначе ищем совпадающее имя ниже вершины, чтобы определить характер нарушения.
        const auto matching = std::find_if(
            stack.rbegin(),
            stack.rend(),
            [&](const OpenElement& element) {
                return element.name == token.name;
            });

        // Совпадения нет во всём стеке: сообщаем о лишнем или неверном закрывающем теге.
        if (matching == stack.rend()) {
            result.diagnostics.push_back({
                "Структурная ошибка",
                "Закрывающий тег </" + token.name +
                    "> не имеет открывающей пары; ожидался закрывающий "
                    "тег для <" + stack.back().name + ">.",
                token.line,
                token.column
            });
            continue;
        }

        result.diagnostics.push_back({
            "Структурная ошибка",
            "Нарушена вложенность: тег </" + token.name +
                "> встретился до закрытия элемента <" +
                stack.back().name + ">.",
            token.line,
            token.column
        });

        // Снимаем ошибочно вложенные элементы до найденной пары.
        // Снимаем незакрытые вложенные элементы до найденной открывающей пары.
        while (!stack.empty() && stack.back().name != token.name) {
            // Сначала сохраняем ссылку на вершину, чтобы сформировать диагностику до удаления.
            const OpenElement& unclosed = stack.back();

            result.diagnostics.push_back({
                "Структурная ошибка",
                "Открывающий тег <" + unclosed.name +
                    "> не закрыт до закрывающего тега </" +
                    token.name + ">.",
                unclosed.line,
                unclosed.column
            });

            stack.pop_back();
        }

        // Удаляем найденную пару, если она сохранилась после восстановления стека.
        if (!stack.empty()) {
            stack.pop_back();
        }
    }

    // Оставшиеся элементы так и не встретили закрывающий тег до конца документа.
    for (const OpenElement& element : stack) {
        result.diagnostics.push_back({
            "Структурная ошибка",
            "Открывающий тег <" + element.name +
                "> не имеет закрывающего тега.",
            element.line,
            element.column
        });
    }
}
