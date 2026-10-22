#pragma once

#include <cstddef>
#include <string>
#include <vector>

struct Input_data {
    std::string result;
    bool Esc_controller = false;
    bool Enter_controller = false;
};

enum class HtmlTokenType {
    OpenTag,
    CloseTag,
    SelfClosingTag,
    Comment,
    Declaration,
    Text
};

struct HtmlAttribute {
    std::string name;
    std::string value;
};

struct HtmlToken {
    HtmlTokenType type = HtmlTokenType::Text;
    std::string name;
    std::string raw_text;
    std::vector<HtmlAttribute> attributes;
    std::size_t offset = 0;
    std::size_t line = 1;
    std::size_t column = 1;
};

struct HtmlDiagnostic {
    std::string category;
    std::string message;
    std::size_t line = 1;
    std::size_t column = 1;
};

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
