related_commands: get_config_no_dialog

demo_code:
```python
s = sioyek.Sioyek('', launch_if_not_exists=True, launch_args=LAUNCH_ARGS)
s.goto_page_with_page_number(1)
start_recording(RECORDING_FILE_NAME)
show_run_command(s, 'get_config_value')
type_words(s, "highlight_color_a")
time.sleep(3)
end_recording()
```

Displays the vlue of the given config string to the user.