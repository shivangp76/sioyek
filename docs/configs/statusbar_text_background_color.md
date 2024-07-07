related_commands: toggle_statusbar

related_configs: 

demo_code:
```python
s = sioyek.Sioyek('', launch_if_not_exists=True, launch_args=LAUNCH_ARGS)
s.toggleconfig_show_setconfig_in_statusbar()
s.goto_page_with_page_number(1)
s.toggle_custom_color()

start_recording(RECORDING_FILE_NAME)

original_color = ' '.join(s.get_config_no_dialog('status_bar_color').strip().split(' ')[1:])

for color in DARK_COLORS:
    s.setconfig_status_bar_color(color)
    s.reload_config()
    time.sleep(1)

s.setconfig_status_bar_color(original_color)
s.reload_config()
time.sleep(1)

for color in LIGHT_COLORS:
    s.setconfig_status_bar_text_color(color)
    s.reload_config()
    time.sleep(1)

time.sleep(2)

end_recording()
```

for_configs: status_bar_color status_bar_text_color

doc_body:
Text and background colors of statusbar.