related_commands: add_highlight add_highlight_with_current_type

demo_code:
```python
s = sioyek.Sioyek('', launch_if_not_exists=True, launch_args=LAUNCH_ARGS)
s.goto_page_with_page_number(1)
start_recording(RECORDING_FILE_NAME)
show_run_command(s, 'goto_next_highlight')
time.sleep(2)
for i in range(3):
    s.goto_next_highlight()
    time.sleep(2)

show_run_command(s, 'goto_prev_highlight')

for i in range(3):
    s.goto_prev_highlight()
    time.sleep(2)

end_recording()
```

for_commands: goto_next_highlight goto_prev_highlight

doc_body:
Jump to the next/previous highlight in the current document.