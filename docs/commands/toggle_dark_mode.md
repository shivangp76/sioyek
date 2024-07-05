related_commands: 

demo_code:
```python
s = sioyek.Sioyek('', launch_if_not_exists=True, launch_args=LAUNCH_ARGS)
s.goto_page_with_page_number(1)
start_recording(RECORDING_FILE_NAME)
time.sleep(2)
show_run_command(s, 'toggle_dark_mode')
time.sleep(3)
end_recording()
```

for_commands:

doc_body:
Toggle between light and dark color schemes.