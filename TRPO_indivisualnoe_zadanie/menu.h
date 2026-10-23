// ст0раницы интерфейса
#pragma once


#include <cstddef>
#include <string>
#include "Struct.h"

// Показывает начальный экран и ожидает продолжения пользователя.
void show_start_window();

// Отображает главное меню и возвращает код выбранного действия.
int show_main_menu();

// Возвращает имя выбранного HTML-файла; пустая строка обозначает отмену или пустой список.
std::string choose_source_file();

// Показывает справочную информацию о программе и её возможностях.
void show_about_screen();

// Показывает итог завершённой операции: имя файла и количество ошибок.
void show_completion_screen(
    
    const std::string& filename,                // Имя обработанного файла для отображения пользователю.
    
    std::size_t errors);                        // Количество ошибок, найденных при обработке.

// Предлагает сохранить результат анализа после просмотра.
void show_save_prompt(const HtmlAnalysisResult& result, const std::string& source_filename);
