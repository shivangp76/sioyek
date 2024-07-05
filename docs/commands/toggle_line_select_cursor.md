
related_commands: select_ruler_text

demo_code:
```python
s = sioyek.Sioyek('', launch_if_not_exists=True, launch_args=LAUNCH_ARGS)
s.goto_page_with_page_number(1)
s.focus_text('fullsceen')
s.select_ruler_text()
s.move_visual_mark_down()
s.move_visual_mark_down()
s.move_visual_mark_down()
start_recording(RECORDING_FILE_NAME)
show_run_command(s, 'toggle_line_select_cursor')
time.sleep(1)
s.toggle_line_select_cursor()
time.sleep(1)
s.toggle_line_select_cursor()
time.sleep(1)
s.toggle_line_select_cursor()
time.sleep(2)
end_recording()
```

for_commands:

doc_body:
Toggle between begin and end of line text selection by @command(select_ruler_text).