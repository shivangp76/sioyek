related_commands: ruler_under_cursor keyboard_select_line

demo_code:
```python
s = sioyek.Sioyek('', launch_if_not_exists=True, launch_args=LAUNCH_ARGS)
s.goto_page_with_page_number(1)
s.keyboard_select_line('', wait=False)
time.sleep(0.3)
s.send_symbol('c')
start_recording(RECORDING_FILE_NAME)
time.sleep(2)
show_run_command(s, 'exit_ruler_mode')
time.sleep(3)
end_recording()
```

for_commands: exit_ruler_mode close_visual_mark

doc_body:
Exit ruler mode.