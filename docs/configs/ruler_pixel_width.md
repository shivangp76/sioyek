related_commands: ruler_under_cursor focus_text

related_configs: ruler_display_mode

for_configs:

demo_code:
```python
s = sioyek.Sioyek('', launch_if_not_exists=True, launch_args=LAUNCH_ARGS)
s.toggleconfig_show_setconfig_in_statusbar()
s.goto_page_with_page_number(1)
s.focus_text('fullscreen')

start_recording(RECORDING_FILE_NAME)

values = range(10)
for val in values:
    s.setconfig_ruler_pixel_width(f'{val}')
    time.sleep(1)

time.sleep(2)

end_recording()
```

doc_body:
Ruler thickness when @config(ruler_display_mode) is set to `underline`.
