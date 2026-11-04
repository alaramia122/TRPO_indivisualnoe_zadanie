#include "Header.h"

// Приводим буквы имени тега к нижнему регистру для единообразного сравнения.
std::string to_lower_name(std::string value) {

    for (char& ch : value) {                                                // Перебираем каждый символ имени.
        ch = static_cast<char>(
            std::tolower(static_cast<unsigned char>(ch)));                      // Приводим символ к нижнему регистру безопасно для ctype.
    }

    return value;                                                           // Возвращаем строку с нормализованным именем.
}

// Убираем пробельные символы по краям строки; пустая строка остаётся пустой.
std::string trim_spaces(std::string& value) {

    std::size_t left = 0;                                                   // Индекс первого непробельного символа слева.

    while (left < value.size() &&                                           // Пропускаем ведущие пробелы, пока не закончится строка.
        std::isspace(static_cast<unsigned char>(value[left]))) {
        ++left;                                                                 // Сдвигаем левую границу вправо.
    }

    std::size_t right = value.size();                                       // Правая граница изначально указывает за последний символ.

    while (right > left &&                                                  // Пропускаем хвостовые пробелы, не заходя левее left.
        std::isspace(static_cast<unsigned char>(value[right - 1]))) {
        --right;                                                                // Сдвигаем правую границу влево.
    }

    return value.substr(left, right - left);                                // Возвращаем внутренний фрагмент без крайних пробелов.
}

// Снимаем одну пару окружающих кавычек, если значение атрибута в них заключено.
std::string strip_quotes(std::string value) {

    if (value.size() >= 2) {                                                // Проверяем, что строка не короче двух символов.
        char first = value.front();                                             // Первый символ предполагаемой открывающей кавычки.
        char last = value.back();                                               // Последний символ предполагаемой закрывающей кавычки.

        if ((first == '"' && last == '"') ||                                    // Если двойные кавычки по краям.
            (first == '\'' && last == '\'')) {                                  // Либо одинарные кавычки по краям.
            value = value.substr(1, value.size() - 2);                              // Удаляем оба крайних символа-кавычки.
        }
    }

    return value;                                                           // Возвращаем значение без ограничивающих кавычек либо без изменений.
}

// Добавляем диагностику с указанием строки исходного файла.
void add_diagnostic(
    AnalysisResult& result,                                             // Общий результат анализа, в который записывается диагностика.
    const std::string& category,                                              // Категория проблемы, например "Лексическая".
    const std::string& message,                                               // Пояснение, понятное пользователю.
    int line) {                                                         // Номер строки исходного документа.

    Diagnostic item;                                                        // Создаём одну запись диагностики.
    item.category = category;                                               // Сохраняем категорию проблемы.
    item.message = message;                                                 // Сохраняем текстовое описание.
    item.line = line;                                                       // Сохраняем номер строки источника.
    result.diagnostics.push_back(item);                                     // Добавляем запись в общий список диагностик.
}

// Разбираем содержимое между символами '<' и '>' и формируем один токен.
void parse_tag_content(
    std::string& inside,                                                // Текст между угловыми скобками без самих '<' и '>'.
    int line_no,                                                        // Номер строки, на которой найден фрагмент.
    AnalysisResult& result) {                                           // Результат анализа для токенов, счётчиков и диагностик.

    std::string trimmed = trim_spaces(inside);                              // Убираем пробелы по краям перед классификацией фрагмента.

    if (trimmed.empty()) {                                                  // Если строка пустая, то считаем что пустые угловые скобки не образуют корректный тег.
        add_diagnostic(
            result,
            "Лексическая",
            "После символа < ожидается имя тега или конструкция разметки.",
            line_no);
        return;                                                                 // Дальнейший разбор пустого содержимого не требуется.
    }

    // Комментарий HTML начинается с последовательности !-- .
	if (trimmed.size() >= 3 && trimmed.substr(0, 3) == "!--") {             // Если это комментарий
        Token token;                                                            // Создаём токен комментария.
        token.type = MyTokenType::Comment;                                      // Отмечаем тип как комментарий.
        token.text = trimmed;                                                   // Сохраняем исходное содержимое комментария.
        token.line = line_no;                                                   // Привязываем токен к номеру строки.
        result.tokens.push_back(token);                                         // Добавляем токен в общий список.
        ++result.comments;                                                      // Увеличиваем счётчик комментариев.
        return;                                                                 // Комментарий полностью обработан.
    }

    // Декларации и инструкции обработки начинаются с '!' или '?'.
	if (trimmed[0] == '!' || trimmed[0] == '?') {                           // Если это декларация или инструкция обработки
        Token token;                                                            // Создаём токен специальной конструкции.
        token.type = MyTokenType::Other;                                        // Отмечаем тип как прочую разметку.
        token.text = trimmed;                                                   // Сохраняем исходный текст конструкции.
        token.line = line_no;                                                   // Привязываем токен к номеру строки.
        result.tokens.push_back(token);                                         // Добавляем токен в общий список.
        return;                                                                 // Специальная конструкция полностью обработана.
    }

    bool closing = false;                                                   // Признак закрывающего тега.
    bool self_closing = false;                                              // Признак самозакрывающегося тега.
    std::string body = trimmed;                                             // Рабочая копия содержимого тега без крайних пробелов.

    if (body[0] == '/') {                                                   // Начальный '/' означает закрывающий тег.
        closing = true;                                                         // Фиксируем закрывающий тип.
        body.erase(0, 1);                                                       // Удаляем начальный слеш из рабочей копии.
        body = trim_spaces(body);                                               // Убираем пробелы, которые могли стоять после слеша.
    }

    if (body.back() == '/') {                                               // Завершающий '/' означает самозакрывающийся тег.
        self_closing = true;                                                    // Фиксируем самозакрытие.
        body.pop_back();                                                        // Удаляем завершающий слеш из рабочей копии.
        body = trim_spaces(body);                                               // Убираем пробелы, которые могли стоять перед слешем.
    }

    // Делим содержимое тега по пробелам: первый фрагмент — имя, остальные — атрибуты.
    std::vector<std::string> parts = Cutter_of_string(body, ' ');
    std::vector<std::string> pieces;                                        // Список непустых фрагментов после дополнительной очистки.

    for (std::string& part : parts) {                                       // Проходим по всем фрагментам, полученным после разбиения.
		std::string item = trim_spaces(part);                                   // Убираем пробелы по краям каждого фрагмента.
		if (!item.empty()) {                                                    // Если фрагмент не пустой,
            pieces.push_back(item);                                                 // Добавляем его в список.
        }
    }

    if (pieces.empty()) {                                                   // Если после удаления слешей и пробелов имя тега так и не найдено.
		add_diagnostic(                                                         // Сообщаем о некорректной конструкции.
            result,
            "Лексическая",
            "После символа < ожидается имя тега.",
            line_no);
        return;                                                                 // Без имени тег сформировать нельзя.
    }

    Token token;                                                            // Создаём токен тега.
    token.name = to_lower_name(pieces[0]);                                  // Нормализуем имя тега к нижнему регистру.
    token.text = trimmed;                                                   // Сохраняем исходное содержимое между угловыми скобками.
    token.line = line_no;                                                   // Привязываем токен к номеру строки.

    if (!closing) {                                                         // Если тег закрывающий,атрибуты не разбираем.
        for (std::size_t i = 1; i < pieces.size(); ++i) {                       // Проходим по оставшимся фрагментам.

            std::vector<std::string> av = Cutter_of_string(pieces[i], '=');         // Делим фрагмент на имя и значение по первому '='.
            if (av.empty()) {                                                       // Пустой фрагмент после деления пропускаем.
                continue;
            }

            Attribute attr;                                                         // Создаём экземпляр атрибута
            attr.name = trim_spaces(av[0]);                                         // Имя атрибута — левая часть до '='.

            if (av.size() >= 2) {                                                   // Значение присутствует, если после '=' есть хотя бы один фрагмент.
                std::string value = av[1];                                              // Начинаем значение с первого фрагмента после '='.

                for (std::size_t k = 2; k < av.size(); ++k) {                           // Если в значении были дополнительные '=', склеиваем хвост обратно.
                    value += '=';
                    value += av[k];
                }

                attr.value = strip_quotes(trim_spaces(value));                          // Убираем пробелы и окружающие кавычки у значения.
            }

            if (!attr.name.empty()) {                                               // Если атрибут не пуст
                token.attrs.push_back(attr);                                            // Добавляем атрибут в текущий токен.
                ++result.attributes;                                                    // Увеличиваем общий счётчик атрибутов.
            }
        }
    }

    // Тип токена выбираем по признакам закрытия и самозакрытия.
    if (closing) {
        token.type = MyTokenType::Close;                                        // Закрывающий тег.
        ++result.close_tags;                                                    // Увеличиваем счётчик закрывающих тегов.
    }
    else if (self_closing) {
        token.type = MyTokenType::SelfClose;                                    // Самозакрывающийся тег.
        ++result.self_tags;                                                     // Увеличиваем счётчик самозакрывающихся тегов.
    }
    else {
        token.type = MyTokenType::Open;                                         // Обычный открывающий тег.
        ++result.open_tags;                                                     // Увеличиваем счётчик открывающих тегов.
    }

    result.tokens.push_back(token);                                         // Добавляем сформированный токен в общий результат разбора.
}

// Сохраняем текстовый фрагмент, если он не является пустой строкой.
void push_text(
    const std::string& text,                                            // Исходный текстовый фрагмент между тегами.
    int line_no,                                                        // Номер строки, на которой расположен фрагмент.
    AnalysisResult& result) {                                           // Результат анализа для токенов и счётчика текста.

    if (text.empty()) {                                                     // Пустой текст в список токенов не добавляем.
        return;
    }

    Token token;                                                            // Создаём текстовый токен.
    token.type = MyTokenType::Text;                                         // Отмечаем тип как обычный текст.
    token.text = text;                                                      // Сохраняем исходное содержимое фрагмента.
    token.line = line_no;                                                   // Привязываем токен к номеру строки.
    result.tokens.push_back(token);                                         // Добавляем токен в общий список.
    ++result.texts;                                                         // Увеличиваем счётчик текстовых фрагментов.
}

// Разбираем исходные строки файла и заполняем токены, счётчики и лексические диагностики.
void parse_html(
    const std::vector<std::string>& lines,                              // Строки исходного HTML-документа по порядку.
    AnalysisResult& result) {                                           // Результат, который наполняется токенами и диагностиками.

    bool inside_raw = false;                                                // Флаг для определения нахождения внутри script или style.
    std::string raw_name;                                                   // Имя текущего «сырого» контейнера: script или style.

    for (int i = 0; i < lines.size(); ++i) {                                // Обрабатываем файл построчно.

        const std::string& line = lines[i];                                     // Текущая строка файла.
        int line_no = i + 1;                                                    // Нумерация строк для диагностик.
        std::size_t pos = 0;                                                    // Текущая позиция разбора внутри строки.

        while (pos < line.size()) {                                             // Пока в строке остаются необработанные символы.

            if (line[pos] != '<') {                                                 // Обычный текст продолжается до следующего символа '<'.

                std::size_t end = line.find('<', pos);                                  // Ищем начало следующей разметки.
                std::size_t stop = end;                                                 // По умолчанию считаем, что текст заканчивается на найденном '<'.

                if (end == std::string::npos) {                                         // Если '<' в остатке строки нет,
                    stop = line.size();                                                     // текст идёт до конца строки.
                }
                const std::string text = line.substr(pos, stop - pos);                  // Вырезаем текстовый фрагмент.

                if (!inside_raw) {                                                      // Если мы не внутри контейнера script или style
					push_text(text, line_no, result);                                       // сохраняем текстовый токен.
                }

                pos = stop;                                                             // Продвигаем позицию к началу тега или к концу строки.
                continue;                                                               // Переходим к следующей итерации разбора строки.
            }

            const std::size_t end = line.find('>', pos);                            // Ищем закрывающую '>' на этой же строке.

			if (end == std::string::npos) {                                         // Если закрывающей '>' нет, то это ошибка синтаксиса.
                add_diagnostic(
                    result,
                    "Лексическая",
                    "Для тега или комментария ожидается завершающий символ >.",
                    line_no);

                if (!inside_raw) {                                                      // Если мы не внутри контейнера script или style
					push_text(line.substr(pos), line_no, result);                           // Сохраняем хвост как текст, чтобы не терять исходный фрагмент.
                }

                break;                                                                  // Дальше по этой строке разбирать нечего.
            }

            std::string inside = line.substr(pos + 1, end - pos - 1);               // Содержимое между '<' и '>' без самих скобок.
            pos = end + 1;                                                          // Продвигаем позицию сразу за закрывающую скобку.

            if (inside_raw) {                                                       // В режиме script/style ищем только парный закрывающий тег.

                std::string probe = trim_spaces(inside);                                // Рабочая копия содержимого потенциального закрывающего тега.

                if (!probe.empty() && probe[0] == '/') {                                // Закрывающий тег обязан начинаться со слеша.
                    probe.erase(0, 1);                                                      // Удаляем начальный слеш.
                    probe = trim_spaces(probe);                                             // Убираем пробелы после слеша.

                    std::size_t name_end = 0;                                               // Длина имени закрывающего тега.

                    while (name_end < probe.size() &&                                       // Имя продолжается до пробела или дополнительного слеша.
                        !std::isspace(static_cast<unsigned char>(probe[name_end])) &&
                        probe[name_end] != '/') {
                        ++name_end;
                    }

                    std::string name = to_lower_name(probe.substr(0, name_end));            // Нормализуем извлечённое имя.

                    if (name == raw_name) {                                                 // Найден парный закрывающий тег для script/style.
                        Token token;                                                            // Создаём токен закрывающего тега.
                        token.type = MyTokenType::Close;
                        token.name = name;
                        token.text = inside;
                        token.line = line_no;
                        result.tokens.push_back(token);
                        ++result.close_tags;
                        inside_raw = false;                                                     // Выходим из «сырого» режима.
                        raw_name.clear();                                                       // Имя контейнера больше не нужно.
                    }
                }

                continue;                                                               // Прочий текст внутри script/style пропускаем.
            }

            const std::size_t tokens_before = result.tokens.size();                 // Запоминаем число токенов до разбора текущего тега.
            parse_tag_content(inside, line_no, result);                             // Разбираем тег, комментарий или декларацию.

            if (result.tokens.size() > tokens_before) {                             // Если токен действительно добавился, проверяем script/style.
                Token& last = result.tokens.back();                                     // Последний добавленный токен.

				if (last.type == MyTokenType::Open &&                                   // Если это открывающий тег
					(last.name == "script" || last.name == "style")) {                  // и его имя — script или style
                    inside_raw = true;                                                      // Включаем режим пропуска содержимого.
                    raw_name = last.name;                                                   // Запоминаем, какой закрывающий тег ожидается.
                }
            }
        }
    }

    if (inside_raw) {                                                       // Если контейнер script/style так и не был закрыт до конца файла.

		int error_line = 1;                                                     // Номер строки, на которой был открыт контейнер.

        if (!lines.empty()) {                                                   // Если файл не пуст, привязываем диагностику к последней строке.
            error_line = lines.size();
        }

        add_diagnostic(
            result,
            "Лексическая",
            "Не найден закрывающий тег </" + raw_name + ">.",
            error_line);
    }
}