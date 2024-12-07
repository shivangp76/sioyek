related_commands: 

related_configs: 
type: color3

demo_code:
```python
s = sioyek.Sioyek('', launch_if_not_exists=True, launch_args=LAUNCH_ARGS)
s.toggleconfig_show_setconfig_in_statusbar()
s.goto_page_with_page_number(1)

s.zoom_out()
s.zoom_out()
s.zoom_out()

start_recording(RECORDING_FILE_NAME)

s.setconfig_background_color('1 0 0')
time.sleep(1)
s.setconfig_background_color('1 0 1')
time.sleep(1)
s.setconfig_background_color('0 0 1')
time.sleep(3)

s.toggle_dark_mode()
time.sleep(1)

s.setconfig_dark_mode_background_color('1 0 0')
time.sleep(1)
s.setconfig_dark_mode_background_color('1 0 1')
time.sleep(1)
s.setconfig_dark_mode_background_color('0 0 1')
time.sleep(3)

s.toggle_custom_color()
time.sleep(1)

s.setconfig_custom_color_mode_empty_background_color('1 0 0')
time.sleep(1)
s.setconfig_custom_color_mode_empty_background_color('1 0 1')
time.sleep(1)
s.setconfig_custom_color_mode_empty_background_color('0 0 1')
time.sleep(3)

end_recording()
```

for_configs: background_color custom_color_mode_empty_background_color dark_mode_background_color

doc_body:
The empty background color in light/dark/custom color modes.