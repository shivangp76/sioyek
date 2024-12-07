related_commands: 
related_configs: dark_mode_contrast dark_mode_background_color

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

All color configs in sioyek can be specialized for dark mode by adding the `DARK_` prefix. For example, if we want to adjust the `text_highlight_color` for dark mode only, we could set `DARK_text_highlight_color`.