related_commands: goto_highlight goto_highlight_g add_highlight_with_current_type goto_next_highlight goto_next_highlight_of_type delete_highlight change_highlight edit_selected_highlight generic_delete generic_select
related_configs: highlight_color_[a-z]

demo_code:
```python
s = sioyek.Sioyek('', launch_if_not_exists=True, launch_args=LAUNCH_ARGS)
s.goto_page_with_page_number(1)
s.focus_text('fullscreen')
s.select_ruler_text()
s.escape()
start_recording(RECORDING_FILE_NAME)
show_run_command(s, 'add_highlight')
time.sleep(1)
s.send_symbol('r')
time.sleep(3)
s.goto_page_with_page_number(2)
time.sleep(2)
show_run_command(s, 'goto_highlight')
time.sleep(3)
end_recording()

```

for_commands:

doc_body:
Highlight the selected text. Requires a symbol which specifies the color of the highlighted text. For example executing @command(add_highlight) followed by pressing `a` will create a highlight with color @config(highlight_color_a). You can configure the highlight color of all highlights in your config files. The highlights can later be searched using @command(goto_highlight).