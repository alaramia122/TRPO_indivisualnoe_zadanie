#include "Header.h"

// Проверяем строку на соответствие одному из наборов разрешённых символов.
bool contains_only(std::string const& str, char flag) {
    // Символьный код выбирает режим проверки, чтобы вызывающий код мог использовать одну функцию.
    switch (flag) {
    case 'б':
        // Проверяем кириллицу и латиницу.
        return str.find_first_not_of(
            "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ"
            "абвгдеёжзийклмнопрстуфхцчшщъыьэюя"
            "АБВГДЕЁЖЗИЙКЛМНОПРСТУФХЦЧШЩЪЫЬЭЮЯ") == std::string::npos;

    case 'р':
        // Проверяем только кириллицу.
        return str.find_first_not_of(
            "абвгдеёжзийклмнопрстуфхцчшщъыьэюя"
            "АБВГДЕЁЖЗИЙКЛМНОПРСТУФХЦЧШЩЪЫЬЭЮЯ") == std::string::npos;

    case 'а':
        // Проверяем только латиницу.
        return str.find_first_not_of(
            "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ") ==
            std::string::npos;

    case 'ц':
        // Проверяем, что строка содержит только цифры.
        return str.find_first_not_of("0123456789") == std::string::npos;

    case 'з':
        // Исключаем знаки конца предложения.
        return str.find_first_of(R"(.!?"'…)") == std::string::npos;

    case 'с':
        // Исключаем специальные символы и знаки пунктуации.
        return str.find_first_of(R"(,,></|=+&^%$#@*(){}[]№;:'»«\)") ==
            std::string::npos;

    // Неизвестный код режима отклоняется, а не трактуется как успешная проверка.
    default:
        return false;
    }
}

// Делит строку по заданному символу и возвращает все извлечённые фрагменты.
std::vector<std::string> Cutter_of_string(
    const std::string& stroka,
    char razdel) {
    // Строковый поток позволяет читать фрагменты последовательно через разделитель.
    std::stringstream stream(stroka);
    std::string part;
    std::vector<std::string> result;

    // Пока поток извлекает очередной фрагмент, добавляем его в результат.
    while (getline(stream, part, razdel)) {
        result.push_back(part);
    }

    return result;
}

// Разбивает строку по разделителю и сохраняет первые символы непустых фрагментов.
std::vector<char> Cutter_of_string_char(
    const std::string& stroka,
    char razdel) {
    std::stringstream stream(stroka);
    std::string part;
    std::vector<char> result;

    // Обрабатываем каждый фрагмент отдельно, пока не исчерпан входной поток.
    while (getline(stream, part, razdel)) {
        // Проверка нужна перед front(): у пустого фрагмента нет первого символа.
        if (!part.empty()) {
            result.push_back(part.front());
        }
    }

    return result;
}

// Считывает клавиши напрямую и возвращает строку вместе с состоянием Enter/Esc.
Input_data input_data_reader(bool flag, bool flag2, bool flag3) {
    Input_data data;
    char ch;

    // Цикл завершается только после того, как обработчик Enter или Esc сбросит флаг продолжения.
    do {
        // Читаем клавишу без автоматического отображения символа и ожидания Enter.
        ch = _getch();

        // Отдельные ветви нужны для управляющих клавиш и обычного текстового ввода.
        switch (ch) {
        // Esc отмечает отмену и останавливает цикл чтения.
        case 27:
            data.Esc_controller = true;
            flag = false;
            break;

        // Enter подтверждает текущую строку и останавливает цикл чтения.
        case 13:
            data.Enter_controller = true;
            flag = false;
            break;

        // Backspace удаляет последний введённый символ, если он существует.
        case 8:
            // Защищаем pop_back() и визуальное удаление от работы с пустой строкой.
            if (!data.result.empty()) {
                data.result.pop_back();
                std::cout << "\b \b";
            }
            break;

        // Префикс -32 обозначает расширенную клавишу в используемом Windows-вводе.
        case -32:
            // Если включён специальный режим, считываем следующий код расширенной клавиши.
            if (flag3) {
                ch = _getch();
            } else {
                data.result += ch;

                // Флаг отображения управляет выводом символа, но не его сохранением в строке.
            if (flag2) {
                    std::cout << ch;
                }
            }
            break;

        // Все остальные коды обрабатываются как обычные вводимые символы.
        default:
            data.result += ch;

            if (flag2) {
                std::cout << ch;
            }
            break;
        }
    // Продолжаем чтение, пока ни Enter, ни Esc не завершили ввод.
    } while (flag);

    return data;
}
