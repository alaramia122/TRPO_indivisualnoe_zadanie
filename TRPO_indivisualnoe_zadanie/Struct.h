#pragma once // Защищаем структуры от повторного включения.
#include <cstddef> // Подключаем std::size_t для координат и смещений.
#include <string> // Подключаем строки.
#include <vector> // Подключаем динамические списки.
struct Input_data { // Сохраняем контракт исходного utils.cpp.
    std::string result; // Храним введённую строку.
    bool Esc_controller = false; // Отмечаем отмену через Esc.
    bool Enter_controller = false; // Отмечаем подтверждение через Enter.
}; // Завершаем структуру ввода.
enum class HtmlTokenType { OpenTag, CloseTag, SelfClosingTag, Comment, Declaration, Text }; // Перечисляем виды фрагментов HTML, включая doctype и декларации.
struct HtmlAttribute { std::string name; std::string value; }; // Храним имя и значение атрибута.
struct HtmlToken { // Храним один фрагмент исходного документа.
    HtmlTokenType type = HtmlTokenType::Text; // Задаём тип по умолчанию.
    std::string name; // Храним имя тега.
    std::string raw_text; // Сохраняем исходный фрагмент.
    std::vector<HtmlAttribute> attributes; // Храним атрибуты тега.
    std::size_t offset = 0; // Храним смещение в файле.
    std::size_t line = 1; // Храним номер строки.
    std::size_t column = 1; // Храним номер столбца.
}; // Завершаем описание токена.
struct HtmlDiagnostic { std::string category; std::string message; std::size_t line = 1; std::size_t column = 1; }; // Храним диагностическое сообщение.
struct HtmlAnalysisResult { // Объединяем токены, ошибки и статистику.
    std::vector<HtmlToken> tokens; // Сохраняем фрагменты в исходном порядке.
    std::vector<HtmlDiagnostic> diagnostics; // Сохраняем лексические и структурные ошибки.
    std::size_t opening_tags = 0; // Считаем открывающие теги.
    std::size_t closing_tags = 0; // Считаем закрывающие теги.
    std::size_t self_closing_tags = 0; // Считаем самозакрывающиеся теги.
    std::size_t attributes = 0; // Считаем атрибуты.
    std::size_t comments = 0; // Считаем комментарии.
    std::size_t text_segments = 0; // Считаем непустые текстовые фрагменты.
}; // Завершаем результат анализа.
