#include "Header.h"

namespace {

// Эти HTML-элементы по правилам рассматриваемого подмножества используют одиночную форму.
const std::vector<std::string> void_elements = {
    "area", "base", "br", "col", "embed", "hr", "img",
    "input", "link", "meta", "param", "source", "track", "wbr"
};

// Координаты открытия сохраняются для указания места элемента, ожидающего закрытия.
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
    // Вершина стека соответствует последнему открытому элементу, ожидающему закрытия.
    std::vector<OpenElement> stack;

    // Последовательный проход сохраняет порядок исходных тегов и диагностик.
    for (const HtmlToken& token : result.tokens) {
        // Открывающие теги парных элементов добавляются в стек.
        if (token.type == HtmlTokenType::OpenTag &&
            !is_void_element(token.name)) {
            stack.push_back({ token.name, token.line, token.column });
            continue;
        }

        // Токены других типов сохраняют текущее состояние стека открытых элементов.
        if (token.type != HtmlTokenType::CloseTag) {
            continue;
        }

        // Пустой стек показывает, что текущий закрывающий тег встретился первым.
        if (stack.empty()) {
            result.diagnostics.push_back({
                "Структурная ошибка",
                "Закрывающий тег </" + token.name +
                    "> требует соответствующего открывающего тега.",
                token.line,
                token.column
            });
            continue;
        }

        // Если совпала вершина, вложенность корректна и достаточно снять один элемент.
        if (stack.back().name == token.name) {
            // После регистрации ошибки дальнейшая проверка продолжается с оставшимися элементами.
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

        // Результат поиска описывает лишний закрывающий тег.
        if (matching == stack.rend()) {
            result.diagnostics.push_back({
                "Структурная ошибка",
                "Закрывающий тег </" + token.name +
                    "> требует соответствующую открывающую пару; текущий открытый элемент — "
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
        // Снимаем вложенные элементы, ожидающие закрытия, до найденной открывающей пары.
        while (!stack.empty() && stack.back().name != token.name) {
            // Сначала сохраняем ссылку на вершину, чтобы сформировать диагностику до удаления.
            const OpenElement& unclosed = stack.back();

            result.diagnostics.push_back({
                "Структурная ошибка",
                "Открывающий тег <" + unclosed.name +
                    "> остался открытым до закрывающего тега </" +
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

    // Оставшиеся элементы ожидают закрывающий тег к концу документа.
    for (const OpenElement& element : stack) {
        result.diagnostics.push_back({
            "Структурная ошибка",
            "Открывающий тег <" + element.name +
                "> ожидает закрывающий тег.",
            element.line,
            element.column
        });
    }
}
