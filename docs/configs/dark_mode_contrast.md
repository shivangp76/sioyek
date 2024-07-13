related_commands: toggle_dark_mode

related_configs: 

demo_code:
```python
s = sioyek.Sioyek('', launch_if_not_exists=True, launch_args=LAUNCH_ARGS)
s.toggleconfig_show_setconfig_in_statusbar()
s.goto_page_with_page_number(1)
s.toggle_dark_mode()

start_recording(RECORDING_FILE_NAME)

s.setconfig_dark_mode_contrast(0)
time.sleep(1)

s.setconfig_dark_mode_contrast(0.5)
time.sleep(1)

s.setconfig_dark_mode_contrast(0.8)
time.sleep(1)

s.setconfig_dark_mode_contrast(1)
time.sleep(3)



end_recording()
```

for_configs:

doc_body:
The color contrast in dark mode.
