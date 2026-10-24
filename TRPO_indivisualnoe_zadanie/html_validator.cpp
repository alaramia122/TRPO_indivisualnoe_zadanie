#include "Header.h"

// Эти HTML-элементы используют одиночную форму.
const std::vector<std::string> void_elements = {
    "area", "base", "br", "col", "embed", "hr", "img",
    "input", "link", "meta", "param", "source", "track", "wbr"
};

// Номер строки сохраняется для элемента, ожидающего закрытия.
struct OpenElement {
    std::string name;
    std::size_t line;
};

// Проверяем, относится ли тег к элементам, которым не требуется закрывающая пара.
bool validator_is_void_element(const std::string& name) {
    return std::find(
        void_elements.begin(),
        void_elements.end(),
        name) != void_elements.end();
}

// Сообщение об ошибке.
void validator_add_diagnostic(
    HtmlAnalysisResult& result,
    const std::string& message,
    std::size_t line) {

    result.diagnostics.push_back({
        "Структурная ошибка",
        message,
        line
    });
}

// Проверяем закрывающий тег относительно текущего состояния стека.
void validator_process_closing_tag(
    const HtmlToken& token,
    std::vector<OpenElement>& stack,
    HtmlAnalysisResult& result) {

    // Пустой стек показывает, что закрывающий тег встретился первым.
    if (stack.empty()) {
        validator_add_diagnostic(
            result,
            "Закрывающий тег </" + token.name +
                "> требует соответствующего открывающего тега.",
            token.line);
        return;
    }

    // При совпадении с вершиной достаточно закрыть последний открытый элемент.
    if (stack.back().name == token.name) {
        stack.pop_back();
        return;
    }

    // Ищем совпадающий открывающий тег ниже вершины стека.
    const auto matching = std::find_if(
        stack.rbegin(),
        stack.rend(),
        [&](const OpenElement& element) {
            return element.name == token.name;
        });

    // Если пары нет, сообщаем о лишнем закрывающем теге.
    if (matching == stack.rend()) {
        validator_add_diagnostic(
            result,
            "Закрывающий тег </" + token.name +
                "> требует соответствующую открывающую пару; текущий открытый элемент — "
                "тег для <" + stack.back().name + ">.",
            token.line);
        return;
    }

    // Совпадающий тег найден, но перед ним остались незакрытые вложенные элементы.
    validator_add_diagnostic(
        result,
        "Нарушена вложенность: тег </" + token.name +
            "> встретился до закрытия элемента <" +
            stack.back().name + ">.",
            token.line);

    // Фиксируем каждый пропущенный закрывающий тег.
    while (!stack.empty() && stack.back().name != token.name) {
        const OpenElement unclosed = stack.back();

        validator_add_diagnostic(
            result,
            "Открывающий тег <" + unclosed.name +
                "> остался открытым до закрывающего тега </" +
                token.name + ">.",
            unclosed.line);

        stack.pop_back();
    }

    // Удаляем найденную открывающую пару после обработки вложенний.
    if (!stack.empty()) {
        stack.pop_back();
    }
}

// Добавляем диагностики для элементов, которые остались открытыми в конце документа.
void validator_report_unclosed_elements(
    const std::vector<OpenElement>& stack,
    HtmlAnalysisResult& result) {
    for (const OpenElement& element : stack) {
        validator_add_diagnostic(
            result,
            "Открывающий тег <" + element.name +
                "> ожидает закрывающий тег.",
            element.line);
    }
}

// Проверяем вложенность парных тегов после лексического разбора исходного документа.
void validate_html_structure(HtmlAnalysisResult& result) {
    // Вершина стека соответствует последнему открытому элементу, ожидающему закрытия.
    std::vector<OpenElement> stack;

    // Последовательный проход сохраняет порядок исходных тегов и диагностик.
    for (const HtmlToken& token : result.tokens) {
        if (token.type == HtmlTokenType::OpenTag &&
            !validator_is_void_element(token.name)) {

            stack.push_back({ token.name, token.line });

        } else if (token.type == HtmlTokenType::CloseTag) {

            validator_process_closing_tag(token, stack, result);
        }
    }

    validator_report_unclosed_elements(stack, result);
}
