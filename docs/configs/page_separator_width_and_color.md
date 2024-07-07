related_commands:

related_configs: 

demo_code:
```python
s = sioyek.Sioyek('', launch_if_not_exists=True, launch_args=LAUNCH_ARGS)
s.toggleconfig_show_setconfig_in_statusbar()
s.goto_page_with_page_number(1)
s.goto_bottom_of_page()
s.screen_down()

start_recording(RECORDING_FILE_NAME)
time.sleep(1)

for w in [1, 2, 3, 4, 5]:
    s.setconfig_page_separator_width(str(w))
    time.sleep(1)

for color in DARK_COLORS:
    s.setconfig_page_separator_color(color)
    time.sleep(1)


time.sleep(2)

end_recording()
```

for_configs: page_separator_width page_separator_color

doc_body:
Color and size of separator between pages.