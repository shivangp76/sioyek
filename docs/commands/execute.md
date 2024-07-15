related_commands: 

demo_code:

for_commands: execute execute_shell_command

doc_body:
Executes the given shell command. The following substrings will be expanded in the command:

- `%{file_path}` will expand to the full path of current file.
- `%{file_name}` will expand to current file name.
- `%{selected_text}` will expand to the current selected text.
- `%{selection_begin_document}` and `%{selection_end_document}` will expand to the document location of the begin and end of current text selection. The format is `page x y`.
- `%{page_number}` will expand to the current page number.
- `%{command_text}` will ask the user for a text input and expand to the input.
- `%{mouse_pos_window}` will expand to the mouse position in the window. The format is `x y`.
- `%{mouse_pos_document}` will expand to the mouse position in the document. The format is `page x y`.
- `%{paper_name}` will expand to the current file's automatically detected paper/document title
- `%{sioyek_path}` will expand to the path of sioyek executable
- `%{local_database}` will expand to the path the local database file
- `%{shared_database}` will expand to the path the shared database file
- `%{selected_rect}` will expand to the selected rectangle in the document. The format is `page x1 y1 x2 y2`.
- `%{zoom_level}` will expand to the current zoom level.
- `%{offset_x}` and `%{offset_y}` will expand to the current absolute offsets
- `%{offset_x_document}` and `%{offset_y_document}` will expand to the current document offsets
- `%{line_text}`: if we are in ruler mode will expand to the text of current ruler line
