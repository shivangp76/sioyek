related_commands: toggle_custom_color

related_configs: 

demo_code:
```python
s = sioyek.Sioyek('', launch_if_not_exists=True, launch_args=LAUNCH_ARGS)
s.toggleconfig_show_setconfig_in_statusbar()
s.goto_page_with_page_number(1)
s.toggle_custom_color()

start_recording(RECORDING_FILE_NAME)

s.setconfig_custom_text_color('1 0 0')
time.sleep(1)
s.setconfig_custom_text_color('1 0 1')
time.sleep(1)
s.setconfig_custom_text_color('0 0 1')
time.sleep(1)

s.setconfig_custom_text_color('1 1 1')
time.sleep(1)

s.setconfig_custom_background_color('1 0 0')
time.sleep(1)
s.setconfig_custom_background_color('1 0 1')
time.sleep(1)
s.setconfig_custom_background_color('0 0 1')
time.sleep(3)


end_recording()
```

for_configs: custom_text_color custom_background_color

doc_body:
Text and background colors in custom color mode.