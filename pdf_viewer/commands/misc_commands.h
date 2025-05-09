#pragma once

#include "commands/base_commands.h"

void register_misc_commands(CommandManager* manager);
void perform_command_after_line_is_selected(std::string cname, MainWidget* widget, std::unique_ptr<Command> cmd);
