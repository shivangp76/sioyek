demo_code:
```python
s = sioyek.Sioyek('', launch_if_not_exists=True, launch_args=LAUNCH_ARGS)
s.goto_page_with_page_number(1)
start_recording(RECORDING_FILE_NAME)
show_run_command(s, 'toggle_rect_hints')
time.sleep(3)
end_recording()
```

doc_body:
Shows the locations of touch mode shortcut rectangles and the commands that they perform.