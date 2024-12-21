related_commands: add_marked_bookmark add_bookmark delete_bookmark edit_visible_bookmark edit_selected_bookmark generic_delete generic_select
related_configs: new_shell_bookmark_command

demo_code:
```python
s = sioyek.Sioyek(launch_if_not_exists=True, launch_args=LAUNCH_ARGS)
s.goto_page_with_page_number(1)
start_recording(RECORDING_FILE_NAME)
show_run_command(s, 'add_freetext_bookmark')
time.sleep(1)
s.escape()
s.toggleconfig_keyboard_point_selection()

s.add_freetext_bookmark(wait=False)
pause_recording()
time.sleep(0.1)
s.send_symbol('c')
s.send_symbol('a')
s.send_symbol('h')
s.send_symbol('e')
resume_recording()
time.sleep(0.3)
type_words(s, 'This is a freetext bookmark', delay=0.03)

s.add_freetext_bookmark(wait=False)
pause_recording()
time.sleep(0.1)
s.send_symbol('h')
s.send_symbol('a')
s.send_symbol('m')
s.send_symbol('e')
resume_recording()
time.sleep(0.3)
type_words(s, '#b', final_delay=1)

s.add_freetext_bookmark(wait=False)
pause_recording()
time.sleep(0.1)
s.send_symbol('m')
s.send_symbol('a')
s.send_symbol('r')
s.send_symbol('e')
resume_recording()
time.sleep(0.3)
type_words(s, '#markdown This is a **markdown** <span style="color:red">bookmark</span>', delay=0.03)

s.add_freetext_bookmark(wait=False)
pause_recording()
time.sleep(0.1)
s.send_symbol('a')
s.send_symbol('e')
s.send_symbol('e')
s.send_symbol('n')
resume_recording()
time.sleep(0.3)
type_words(s, '#latex x=5 \implies 2^x-1=31')
 
time.sleep(3)


s.goto_page_with_page_number(1)
time.sleep(1)
show_run_command(s, 'goto_bookmark')
time.sleep(1)
s.control_menu('select')
time.sleep(3)
end_recording()

```

for_commands: add_freetext_bookmark add_freetext_bookmark_auto

doc_body:
Add a freetext bookmark in the selected rectangle. The @command(add_freetext_bookmark) prompts the user to select the rectangle while the @command(add_freetext_bookmark_auto) command doesn't ask for a rectangle, instead it automatically detects possible large empty spaces in the current screen and asks the user to pick one of those. Freetext bookmarks are visible in the document. You can change the display style of the bookmark using the following prefixes:

- If the bookmark text starts with `#` followed by a symbol, it will draw a box with the symbol's color (the same color as the highlighs of the symbol).
- If the bookmark text starts with `#markdown`, the rest of the bookmark will be rendered as markdown
- If the bookmark text starts with `#latex`, the rest of the bookmark will be rendered as latex
- You can change the border/background/text color of the bookmark by adding the corresponding symbol prefix after the `#` or `#markdown` or `#latex` prefix. For example, `#rgb this is bookmark text` will create a bookmark with a red border and green background and blue text. Similarly you can edit the colors of a markdown bookmark like this: `#markdownrgb this is **markdown** bookmark text`.
- If the bookmarks text starts with `? `, it will be interpreted as a question to be asked from the current document (using a large language model). The response will be displayed in the bookmark (this feature is only available to pro users).
- If the bookmark text starts with `#shell` followed by a shell command, then sioyek will run the shell command and fill the bookmark with the output of the shell command. The following variables will be expanded in the shell command:
    * `%{text}` will expand to the text of the bookmark after the `#shell` line (following the newline character). For example, `#shell ping %{text}\ngoogle.com `
    will run `ping google.com` and display the output in the bookmark. Obviously, this is not useful when running the `#shell` bookmark directly but it is useful when defining predefined shell commands using @config(new_shell_bookmark_command).
    * `%{document_text_file}` expands to the path of a temporary file containing the current document's text.
    * `%{current_page_begin_index}` expands to the index of the first character of the current page in the document text file.
    * `%{current_page_end_index}` expands to the index of the last character of the current page in the document text file.
    * `%{bookmark_image_file}` expands to the path of a temporary file containing the image of the document section where the bookmark is located.
    * `%{selected_text}` expands to the text of the selected text in the document.
- If the bookmark text starts with `@cmd` where `cmd` is a command previously defined using @config(new_shell_bookmark_command), then sioyek will run the command and fill the bookmark with the output of the command.