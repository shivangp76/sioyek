related_commands: toggle_fullscreen

demo_code:
```python
s = sioyek.Sioyek('', launch_if_not_exists=True, launch_args=LAUNCH_ARGS)
s.goto_page_with_page_number(1)
#s.move_window('200 200 500 500')
start_recording(RECORDING_FILE_NAME)
show_run_command(s, 'maximize')
time.sleep(3)
end_recording()
```

for_commands:

doc_body:
Maximize the sioyek window.