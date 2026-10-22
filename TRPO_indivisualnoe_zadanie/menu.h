#pragma once

#include <cstddef>
#include <string>

void show_start_window();
int show_main_menu();
std::string choose_source_file();
void show_about_screen();
void show_completion_screen(
    const std::string& filename,
    std::size_t errors);
