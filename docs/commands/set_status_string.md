related_commands:

demo_code:
```python
s = sioyek.Sioyek('', launch_if_not_exists=True, launch_args=LAUNCH_ARGS)
s.goto_page_with_page_number(1)
start_recording(RECORDING_FILE_NAME)
time.sleep(1)
show_run_command(s, 'set_status_string')
time.sleep(2)
type_words(s, 'Hello from sioyek statusbar')
time.sleep(3)
show_run_command(s, 'clear_status_string')
time.sleep(3)

end_recording()
```

for_commands: set_status_string clear_status_string

doc_body:
Set the message in sioyek status bar. This message will be displayed until it is cleared using @command(clear_status_string). This is mainly useful for scripts and plugins to display messages to the user.