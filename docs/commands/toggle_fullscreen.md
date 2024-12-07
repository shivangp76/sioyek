related_commands: toggle_statusbar toggle_scrollbar

demo_code:
```python
s = sioyek.Sioyek('', launch_if_not_exists=True, launch_args=LAUNCH_ARGS)
s.goto_page_with_page_number(1)
start_recording(RECORDING_FILE_NAME)
show_run_command(s, 'toggle_fullscreen')
time.sleep(3)
end_recording()
```

for_commands:

doc_body:
Toggle fullscreen mode.