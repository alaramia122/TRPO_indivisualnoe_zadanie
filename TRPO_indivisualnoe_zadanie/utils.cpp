#include "Header.h"

// Проверяем строку на соответствие одному из наборов разрешённых символов.
bool contains_only(std::string const& str, char flag) {
    
    switch (flag) {                                                     // Символьный код выбирает режим проверки.
    case 'б':                                                               // Проверяем кириллицу и латиницу.
        return str.find_first_not_of(
            "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ"
            "абвгдеёжзийклмнопрстуфхцчшщъыьэюя"
            "АБВГДЕЁЖЗИЙКЛМНОПРСТУФХЦЧШЩЪЫЬЭЮЯ") == std::string::npos;

    case 'р':                                                               // Проверяем только кириллицу.
        return str.find_first_not_of(
            "абвгдеёжзийклмнопрстуфхцчшщъыьэюя"
            "АБВГДЕЁЖЗИЙКЛМНОПРСТУФХЦЧШЩЪЫЬЭЮЯ") == std::string::npos;

    case 'а':                                                               // Проверяем только латиницу.
        return str.find_first_not_of(
            "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ") ==
            std::string::npos;

    case 'ц':                                                               // Проверяем, что строка содержит только цифры.
        return str.find_first_not_of("0123456789") == std::string::npos;

    case 'з':                                                               // Исключаем знаки конца предложения.
        return str.find_first_of(R"(.!?"'…)") == std::string::npos;

    case 'с':                                                               // Исключаем специальные символы и знаки пунктуации.
        return str.find_first_of(R"(,,></|=+&^%$#@*(){}[]№;:'»«\)") ==
            std::string::npos;

    // Неизвестный код режима отклоняется
    default:
        return false;
    }
}

// Делит строку по заданному символу и возвращает все извлечённые фрагменты.
std::vector<std::string> Cutter_of_string(
    const std::string& stroka,
    char razdel) {
    
    std::stringstream stream(stroka);                                       // Строковый поток позволяет читать фрагменты последовательно через разделитель.
    std::string part;
    std::vector<std::string> result;

    
    while (getline(stream, part, razdel)) {                                 // Пока поток извлекает очередной фрагмент, добавляем его в результат.
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

    
    do {                                    // Цикл завершается только после того, как обработчик Enter или Esc сбросит флаг продолжения.
        
        ch = _getch();                          // Читаем клавишу без автоматического отображения символа и ожидания Enter.

        
        switch (ch) {                           // Отдельные ветви нужны для управляющих клавиш и обычного текстового ввода.
        
        case 27:                                // Esc отмечает отмену и останавливает цикл чтения.
            data.Esc_controller = true;
            flag = false;
            break;

        
        case 13:                                // Enter подтверждает текущую строку и останавливает цикл чтения.
            data.Enter_controller = true;
            flag = false;
            break;

        
        case 8:                                 // Backspace удаляет последний введённый символ, если он существует.
            
            if (!data.result.empty()) {             // Защищаем pop_back() и визуальное удаление от работы с пустой строкой.
                data.result.pop_back();
                std::cout << "\b \b";
            }
            break;

        
        case -32:                               // Префикс -32 обозначает расширенную клавишу в используемом Windows-вводе.
            
            if (flag3) {                            // Если включён специальный режим, считываем следующий код расширенной клавиши.
                ch = _getch();
            }
            else {
                data.result += ch;

                
                if (flag2) {                        // Флаг отображения управляет выводом символа, но не его сохранением в строке.
                    std::cout << ch;
                }
            }
            break;

        
        default:                                // Все остальные коды обрабатываются как обычные вводимые символы.
            data.result += ch;

            
            if (flag2) {                            // Отображаем обычный символ только при включённом выводе введённого текста.
                std::cout << ch;
            }
            break;
        }

    } while (flag);                        // Продолжаем чтение, пока ни Enter, ни Esc не завершили ввод.

    return data;
}
