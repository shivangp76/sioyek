related_commands:

related_configs: 

demo_code:
```python
s = sioyek.Sioyek('', launch_if_not_exists=True, launch_args=LAUNCH_ARGS)
s.toggleconfig_show_setconfig_in_statusbar()
s.goto_page_with_page_number(1)
s.toggle_custom_color()

start_recording(RECORDING_FILE_NAME)
s.command(wait=False)
time.sleep(1)

original_color = ' '.join(str(x) for x in s.conf('ui_background_color'))

for color in DARK_COLORS:
    s.setconfig_ui_background_color(color)
    s.reload_config()
    time.sleep(1)

s.setconfig_ui_background_color(original_color)
s.reload_config()
time.sleep(1)

for color in LIGHT_COLORS:
    s.setconfig_ui_text_color(color)
    s.reload_config()
    time.sleep(1)

s.setconfig_ui_text_color('1 1 1')
s.reload_config()
time.sleep(1)

for color in LIGHT_COLORS:
    s.setconfig_ui_selected_background_color(color)
    s.reload_config()
    time.sleep(1)

for color in DARK_COLORS:
    s.setconfig_ui_selected_text_color(color)
    s.reload_config()
    time.sleep(1)

time.sleep(2)

end_recording()
```

for_configs: ui_text_color ui_background_color ui_selected_text_color ui_selected_background_color

doc_body:
Text and background colors of ui menus.