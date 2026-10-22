#include "Header.h"

namespace {

const std::vector<std::string> void_elements = {
    "area", "base", "br", "col", "embed", "hr", "img",
    "input", "link", "meta", "param", "source", "track", "wbr"
};

struct OpenElement {
    std::string name;
    std::size_t line;
    std::size_t column;
};

bool is_void_element(const std::string& name) {
    return std::find(
        void_elements.begin(),
        void_elements.end(),
        name) != void_elements.end();
}

}

void validate_html_structure(HtmlAnalysisResult& result) {
    std::vector<OpenElement> stack;

    for (const HtmlToken& token : result.tokens) {
        if (token.type == HtmlTokenType::OpenTag &&
            !is_void_element(token.name)) {
            stack.push_back({ token.name, token.line, token.column });
            continue;
        }

        if (token.type != HtmlTokenType::CloseTag) {
            continue;
        }

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

        if (stack.back().name == token.name) {
            stack.pop_back();
            continue;
        }

        // Ищем соответствующий открывающий тег глубже в стеке.
        const auto matching = std::find_if(
            stack.rbegin(),
            stack.rend(),
            [&](const OpenElement& element) {
                return element.name == token.name;
            });

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
        while (!stack.empty() && stack.back().name != token.name) {
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

        if (!stack.empty()) {
            stack.pop_back();
        }
    }

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
