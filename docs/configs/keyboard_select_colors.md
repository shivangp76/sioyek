related_commands: open_link keyboard_select

related_configs: tag_font_face keyboard_point_selection keyboard_select_font_size

for_configs: keyboard_select_background_color keyboard_select_text_color keyboard_selected_tag_background_color keyboard_selected_tag_text_color

demo_code:
```python
s = sioyek.Sioyek('', launch_if_not_exists=True, launch_args=LAUNCH_ARGS)
s.toggleconfig_show_setconfig_in_statusbar()
s.goto_page_with_page_number(1)

start_recording(RECORDING_FILE_NAME)
s.keyboard_select('', wait=False)
time.sleep(0.3)
type_words(s, 'aa ', select=False)

time.sleep(2)


for color in LIGHT_COLORS:
    s.setconfig_keyboard_select_background_color(color+ ' 1')
    time.sleep(2)

for color in DARK_COLORS:
    s.setconfig_keyboard_select_text_color(color + ' 1')
    time.sleep(2)

for color in DARK_COLORS:
    s.setconfig_keyboard_selected_tag_background_color(color + ' 1')
    time.sleep(2)

for color in LIGHT_COLORS:
    s.setconfig_keyboard_selected_tag_text_color(color + ' 1')
    time.sleep(2)

    

end_recording()
```

doc_body:
The color of the tags displayed for keyboard commands.
