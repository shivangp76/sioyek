related_commands: move_left move_right

demo_code:
```python
s = sioyek.Sioyek('', launch_if_not_exists=True, launch_args=LAUNCH_ARGS)
s.goto_page_with_page_number(3)
s.zoom_in()
s.zoom_in()
s.zoom_in()
start_recording(RECORDING_FILE_NAME)
show_run_command(s, 'goto_left')
time.sleep(2)
show_run_command(s, 'goto_right')
time.sleep(2)
show_run_command(s, 'goto_left_smart')
time.sleep(2)
show_run_command(s, 'goto_right_smart')
time.sleep(2)
end_recording()
```

for_commands: goto_left goto_right goto_left_smart goto_right_smart

doc_body:
Move to the left/right side of the document. The `_smart` variants ignore the white page margins.