#include "Header.h"

// Проверяем порядок вложенности открывающих и закрывающих тегов стековым алгоритмом.
// Самозакрывающиеся теги, комментарии, текст и декларации в стек не попадают.
void validate_structure(AnalysisResult& result) {                       // На входе - результат парсинга

    std::vector<std::string> stack;                                         // Вектор с именами открытых парных тегов в порядке вложенности. Выполняет роль стека.
    std::vector<int> open_lines;                                            // Строки, в которых соответствующие теги были открыты.

    for (Token& token : result.tokens) {                                    // Просматриваем токены в порядке их появления в документе.

        switch (token.type) {                                                   // Тип токена определяет, как он влияет на стек.

        case MyTokenType::Open:                                                     // Обычный открывающий тег ожидает парное закрытие.
            stack.push_back(token.name);                                                // Кладём имя тега на вершину стека.
            open_lines.push_back(token.line);                                           // Запоминаем строку открытия для возможной диагностики.
            break;

        case MyTokenType::Close:                                                    // Закрывающий тег должен соответствовать одному из открытых тегов.

			if (stack.empty()) {                                                        // Если нет открытых тегов
				Diagnostic item;                                                            // Создаём диагностическое сообщение
                item.category = "Структурная";
                item.message = "Лишний закрывающий тег </" + token.name + ">.";
                item.line = token.line;
                result.diagnostics.push_back(item);
            }
            else if (stack.back() == token.name) {                                      // Если имя совпало с вершиной стека — обычное корректное закрытие.
                stack.pop_back();
                open_lines.pop_back();
            }
			else {                                                                      // Если имя не совпало с вершиной стека — это ошибка вложенности.
                // Ищем такой же открывающий тег глубже стека.
                int found = -1;                                                             // Индекс найденного открывающего тега; -1 означает, что совпадения нет.

				for (int i = stack.size() - 1; i >= 0; --i) {                               // Просматриваем стек с вершины вниз.
					if (stack[static_cast<std::size_t>(i)] == token.name) {                     // Если нашли открывающий тег с таким же именем.
                        found = i;                                                                  // Берём ближайший снизу подходящий тег.
                        break;
                    }
                }

                if (found >= 0) {                                                           // Если найден более ранний открывающий тег.

                    // Всё, что лежало выше найденного тега, считаем незакрытым.
					for (int i = stack.size() - 1; i > found; --i) {                            // Просматриваем стек сверху вниз до найденного тега.
						Diagnostic item;                                                            // Создаём диагностическое сообщение
                        item.category = "Структурная";
                        item.message = "Незакрытый тег <" + stack[i] + ">.";
                        item.line = open_lines[i];
                        result.diagnostics.push_back(item);
                    }

                    // Дополнительно фиксируем нарушение порядка вложенности.
					Diagnostic order;                                                           // Создаём диагностическое сообщение
                    order.category = "Структурная";
                    order.message = "Ожидался </" + stack.back() + ">, получен </" + token.name + ">.";
                    order.line = token.line;
                    result.diagnostics.push_back(order);

                    // Снимаем со стека найденный тег и всё, что было выше него.
                    stack.resize(found);
                    open_lines.resize(found);
                }
                else {                                                                      // Если подходящего открывающего тега в стеке нет.
					Diagnostic item;                                                            // Создаём диагностическое сообщение
                    item.category = "Структурная";
                    item.message = "Лишний закрывающий тег </" + token.name + ">.";
                    item.line = token.line;
                    result.diagnostics.push_back(item);
                    // Стек не меняем: открытые теги остаются ждать свои закрывающие.
                }
            }
            break;

        case MyTokenType::SelfClose:                                                // Самозакрывающийся тег не создаёт незакрытой пары.
        case MyTokenType::Comment:                                                  // Комментарии на вложенность не влияют.
        case MyTokenType::Text:                                                     // Текстовые фрагменты на вложенность не влияют.
        case MyTokenType::Other:                                                    // Декларации и специальные конструкции на вложенность не влияют.
            break;
        }
    }

    // Всё, что осталось в стеке после прохода, считается незакрытым тегом.
    for (std::size_t i = 0; i < stack.size(); ++i) {                        // Для каждого незакрытого тега
		Diagnostic item;                                                        // Создаём диагностическое сообщение.
        item.category = "Структурная";
        item.message = "Незакрытый тег <" + stack[i] + ">.";
        item.line = open_lines[i];                                          // Указываем строку, где тег был открыт.
        result.diagnostics.push_back(item);
    }
}