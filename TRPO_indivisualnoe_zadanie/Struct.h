// Объявления типов доступны при подключении заголовка из разных модулей.
#pragma once

// Размеры и смещения хранятся в типе std::size_t.
#include <cstddef>
// Строки используются для имён, значений, исходного текста и сообщений.
#include <string>
// Векторы хранят списки атрибутов, токенов и диагностик.
#include <vector>

// Результат чтения клавиатурного ввода и признаки завершения ввода.
struct Input_data {
    std::string result;
    bool Esc_controller = false;
    bool Enter_controller = false;
};

// Категория распознанного фрагмента исходного HTML-документа.
enum class HtmlTokenType {
    // Обычный открывающий тег, например <div>.
    OpenTag,
    // Закрывающий тег, например </div>.
    CloseTag,
    // Самозакрывающийся тег, например <item/>.
    SelfClosingTag,
    // Комментарий HTML, ограниченный маркерами <!-- и -->.
    Comment,
    // Декларация или специальная конструкция, начинающаяся с <! или <?.
    Declaration,
    // Текстовый фрагмент между распознанными тегами и декларациями.
    Text
};

// Имя и значение одного атрибута открывающего HTML-тега.
struct HtmlAttribute {
    // Имя атрибута с обрезанными пробелами по краям.
    std::string name;
    // Значение атрибута после снятия ограничивающих кавычек.
    std::string value;
};

// Токен хранит тип, исходный фрагмент и координаты для последующей проверки.
struct HtmlToken {
    HtmlTokenType type = HtmlTokenType::Text;
    std::string name;
    std::string raw_text;
    std::vector<HtmlAttribute> attributes;
    std::size_t offset = 0;
    std::size_t line = 1;
    std::size_t column = 1;
};

// Одна обнаруженная проблема с текстовым описанием и координатами источника.
struct HtmlDiagnostic {
    std::string category;
    std::string message;
    std::size_t line = 1;
    std::size_t column = 1;
};

// Совокупный результат разбора: токены, диагностики и счётчики элементов документа.
struct HtmlAnalysisResult {
    std::vector<HtmlToken> tokens;
    std::vector<HtmlDiagnostic> diagnostics;
    std::size_t opening_tags = 0;
    std::size_t closing_tags = 0;
    std::size_t self_closing_tags = 0;
    std::size_t attributes = 0;
    std::size_t comments = 0;
    std::size_t text_segments = 0;
};
