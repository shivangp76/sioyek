#include "commands/command_parser.h"

QString CommandInvocation::command_string() {
    if (command_name.size() > 0) {
        if (command_name[0] != '[') return command_name;
        int index = command_name.indexOf(']');
        return command_name.mid(index + 1);
    }
    return "";
}

QString CommandInvocation::mode_string() {
    if (command_name.size() > 0) {
        if (command_name[0] == '[') {
            int index = command_name.indexOf(']');
            return command_name.mid(1, index - 1);
        }
        else {
            return "";
        }
    }
    return "";
}

void ParseState::skip_whitespace() {
    while (index < str.size() && str[index].isSpace()) {
        index++;
    }
}

void ParseState::skip_whitespace_and_commas() {

    while (index < str.size() && (str[index].isSpace() || str[index] == ',')) {
        index++;
    }
}

std::optional<QString> ParseState::next_arg() {
    skip_whitespace();
    QString arg;

    if (str[index] == '\'') {
        index++;
        bool is_prev_char_backslash = false;

        while (index < str.size()) {
            auto ch = str[index];
            if (is_prev_char_backslash) {
                if (ch == '\'') {
                    arg.push_back('\'');
                }
                if (ch == '\\') {
                    arg.push_back('\\');
                }
                if (ch == 'n') {
                    arg.push_back('\n');
                }
                is_prev_char_backslash = false;
            }
            else {
                if (ch == '\\') {
                    is_prev_char_backslash = true;
                    index++;
                    continue;
                }
                if (ch == '\'') {
                    index++;
                    return arg;
                }
                else {
                    arg.push_back(ch);
                }
            }
            index++;
        }
    }
    else {
        while ((str[index] != ')') && (str[index] != ',')) {
            arg.push_back(str[index]);
            index++;
        }
        if (!eof() && str[index] != ')') {
            index++;
        }
        return arg;
    }
    return {};
}

bool ParseState::is_valid_command_name_char(QChar c) {
    return c.isLetterOrNumber() || c == '_' || c == '$' || c == '[' || c == ']';
}

std::optional<CommandInvocation>  ParseState::next_command() {
    QString command_name;
    QStringList command_args;
    skip_whitespace();
    if (index >= str.size()) return {};

    while (index < str.size() && is_valid_command_name_char(str[index])) {
        command_name.push_back(str[index]);
        index++;
    }

    if (index == str.size() || (str[index] != '(')) {
        if (command_name.size() > 0) return CommandInvocation{ command_name , {} };
        return {};
    }


    index++; // skip the (
    while (index < str.size() && str[index] != ')') {
        std::optional<QString> maybe_arg = next_arg();
        if (maybe_arg.has_value()) {
            command_args.push_back(maybe_arg.value());
        }
        else {
            break;
        }
        skip_whitespace_and_commas();
    }
    if (!eof() && str[index] == ')') {
        index++;
    }
    return CommandInvocation{ command_name, command_args };

}

bool ParseState::eof() {
    return index >= str.size();
}

QChar ParseState::cc() {
    return str[index];
}

std::vector<CommandInvocation> ParseState::parse_next_macro_command() {
    std::vector<CommandInvocation> res;

    while (auto cmd = next_command()) {
        res.push_back(cmd.value());
        skip_whitespace();
        if (eof()) break;
        if (cc() == ';') {
            index++;
        }
    }

    return res;
}
