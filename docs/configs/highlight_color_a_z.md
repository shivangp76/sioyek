related_commands: add_highlight

related_configs:
type: color3

for_configs: highlight_color_[a-z]

demo_code:
```python
s = sioyek.Sioyek('', launch_if_not_exists=True, launch_args=LAUNCH_ARGS)
s.toggleconfig_show_setconfig_in_statusbar()
s.goto_page_with_page_number(1)

start_recording(RECORDING_FILE_NAME)
time.sleep(1)

for color in LIGHT_COLORS:
    s.setconfig_highlight_color_b(color)
    time.sleep(2)


end_recording()
```

doc_body:
Specifies the color for each of highlight types. For example `highlight_color_r` specifies the color of highlights with symbol `r`.