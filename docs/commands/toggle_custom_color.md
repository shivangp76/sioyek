related_commands: 

related_configs: custom_text_color custom_background_color custom_color_mode_empty_background_color

demo_code:
```python
s = sioyek.Sioyek('', launch_if_not_exists=True, launch_args=LAUNCH_ARGS)
s.goto_page_with_page_number(1)
start_recording(RECORDING_FILE_NAME)
time.sleep(2)
show_run_command(s, 'toggle_custom_color')
time.sleep(3)
end_recording()
```

for_commands:

doc_body:
Toggle between light and custom color schemes. The background and text color of custom color scheme can be configured using @config(custom_text_color) and @config(custom_background_color).