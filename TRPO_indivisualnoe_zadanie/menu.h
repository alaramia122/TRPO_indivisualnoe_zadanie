#pragma once // Защищаем объявления меню.
#include <string> // Подключаем строки.
#include <cstddef> // Подключаем std::size_t.
void show_start_window(); // Показываем стартовый экран.
int show_main_menu(); // Возвращаем 1 — анализ, 2 — информация, 3 — выход.
std::string choose_source_file(); // Возвращаем имя выбранного HTML-файла либо пустую строку.
void show_about_screen(); // Показываем сведения о программе.
void show_completion_screen(const std::string& filename, std::size_t errors); // Показываем итог проверки.
