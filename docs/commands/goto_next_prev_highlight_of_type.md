related_commands: add_highlight add_highlight_with_current_type

demo_code:
```python
s = sioyek.Sioyek('', launch_if_not_exists=True, launch_args=LAUNCH_ARGS)
s.goto_page_with_page_number(1)
start_recording(RECORDING_FILE_NAME)
show_run_command(s, 'set_select_highlight_type')
time.sleep(1)
s.send_symbol('j')
show_run_command(s, 'goto_next_highlight_of_type')
time.sleep(3)

end_recording()
```

for_commands: goto_next_highlight_of_type goto_prev_highlight_of_type

doc_body:
Jump to the next/previous highlight with the current highlight type (as set by @command(set_select_highlight_type)).