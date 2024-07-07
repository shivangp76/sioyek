related_commands: add_highlight set_select_highlight_type

demo_code:
```python
s = sioyek.Sioyek('', launch_if_not_exists=True, launch_args=LAUNCH_ARGS)
s.goto_page_with_page_number(1)
s.focus_text('fullscreen')
s.select_ruler_text()
s.escape()
start_recording(RECORDING_FILE_NAME)
show_run_command(s, 'add_highlight_with_current_type')
time.sleep(3)
end_recording()
```

for_commands:

doc_body:
Highlight the selected text with the current highlight type as set by @command(set_select_highlight_type).