#pragma once

#include <optional>

#include <qstring.h>
#include <qstringlist.h>

struct CommandInvocation {
    QString command_name;
    QStringList command_args;

    QString command_string();
    QString mode_string();
};

struct ParseState {
    QString str;
    int index = 0;

    void skip_whitespace();
    void skip_whitespace_and_commas();
    std::optional<QString> next_arg();
    bool is_valid_command_name_char(QChar c);
    std::optional<CommandInvocation>  next_command();
    bool eof();
    QChar cc();
    std::vector<CommandInvocation> parse_next_macro_command();

};
