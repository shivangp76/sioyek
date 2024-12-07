related_commands: toggle_dark_mode toggle_custom_color
related_configs:

type: bool

demo_code:
```python
s = sioyek.Sioyek('', launch_if_not_exists=True, launch_args=LAUNCH_ARGS)
s.toggleconfig_show_setconfig_in_statusbar()
s.goto_page_with_page_number(2)

for i in range(4):
    s.zoom_out()

s.toggle_dark_mode()

start_recording(RECORDING_FILE_NAME)

for i in range(3):
    s.setconfig_preserve_image_colors_in_dark_mode('1')
    time.sleep(2)

    s.setconfig_preserve_image_colors_in_dark_mode('0')
    time.sleep(2)

time.sleep(1)
end_recording()
```

for_configs: preserve_image_colors_in_dark_mode inverted_preserved_image_colors 

doc_body:
Preserve original image colors in dark mode and custom color mode.
If @config(inverted_preserved_image_colors) is set, then we invert the original image colors in custom color mode.
