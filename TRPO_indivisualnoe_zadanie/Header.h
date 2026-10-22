#pragma once // Защищаем центральный заголовок.
#include <algorithm> // Подключаем стандартные алгоритмы.
#include <cctype> // Подключаем классификацию символов.
#include <filesystem> // Подключаем работу с путями.
#include <fstream> // Подключаем чтение файлов.
#include <iostream> // Подключаем консольный вывод.
#include <sstream> // Подключаем строковые потоки для utils.cpp.
#include <string> // Подключаем строки.
#include <vector> // Подключаем динамические списки.
#include <Windows.h> // Сохраняем Windows-специфику исходного проекта.
#include <conio.h> // Подключаем _getch() для неизменяемого utils.cpp.
#include "Struct.h" // Подключаем общие структуры.
namespace fs = std::filesystem; // Сокращаем имя файловой системы.
#include "utils.h" // Подключаем существующие функции ввода.
#include "menu.h" // Подключаем интерфейс меню.
#include "file_reader.h" // Подключаем чтение файлов.
#include "html_parser.h" // Подключаем ручной парсер.
#include "html_validator.h" // Подключаем структурный валидатор.
#include "output.h" // Подключаем вывод результата.
