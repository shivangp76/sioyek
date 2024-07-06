related_commands: 

demo_code:
```python
s = sioyek.Sioyek('', launch_if_not_exists=True, launch_args=LAUNCH_ARGS)
s.goto_page_with_page_number(3)
start_recording(RECORDING_FILE_NAME)
show_run_command(s, 'rotate_clockwise')
time.sleep(2)
show_run_command(s, 'rotate_counterclockwise')
time.sleep(2)
end_recording()
```

for_commands: rotate_clockwise rotate_counterclockwise

doc_body:
Rotate the document.