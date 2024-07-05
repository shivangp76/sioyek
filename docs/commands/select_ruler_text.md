related_commands: visual_mark_under_cursor focus_text toggle_line_select_cursor

demo_code:
```python
s = sioyek.Sioyek('', launch_if_not_exists=True, launch_args=LAUNCH_ARGS)
s.goto_page_with_page_number(1)
s.focus_text('fullsceen')
start_recording(RECORDING_FILE_NAME)
show_run_command(s, 'select_ruler_text')
time.sleep(1)
s.move_visual_mark_down()
time.sleep(1)
s.move_visual_mark_down()
time.sleep(1)
s.move_visual_mark_down()
time.sleep(1)
s.move_visual_mark_up()
time.sleep(1)
show_run_command(s, 'toggle_line_select_cursor')
time.sleep(1)
s.move_visual_mark_up()
time.sleep(1)
s.move_visual_mark_up()
time.sleep(1)
s.move_visual_mark_down()
time.sleep(1)
s.escape()
time.sleep(2)
end_recording()
```

for_commands:

doc_body:
Select the current line highlighted by ruler, you can use the up and down movement commands to move the selection and @command(toogle_line_select_cursor) to toggle between begin and end of the selection. Exit the selection mode by pressing escape.