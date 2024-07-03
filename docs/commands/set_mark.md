related_commands: goto_mark

doc_body:
Set a mark in the current location so we can return to it later using the @command(goto_mark) command. After executing `set_mark` command, sioyek will wait for you to press a symbol, which can be a lowercase or uppercase letter. Lowercase marks are local to the current file but uppercase marks are global. If you press a symbol that is already used as a mark, the previous mark will be overwritten. You can later return to the mark by executing `goto_mark` command with the same symbol.