related_commands: open_link keyboard_select

related_configs: tag_font_face keyboard_point_selection
type: int

for_configs:

demo_code:
```python
s = sioyek.Sioyek('', launch_if_not_exists=True, launch_args=LAUNCH_ARGS)
s.toggleconfig_show_setconfig_in_statusbar()
s.goto_page_with_page_number(1)

s.keyboard_select(wait=False)

start_recording(RECORDING_FILE_NAME)
time.sleep(2)

vals = [5, 10, 20]
for val in vals:
    s.setconfig_keyboard_select_font_size(f'{val}')
    time.sleep(1)

time.sleep(2)

end_recording()
```

doc_body:
The font size of the tags displayed for keyboard commands.
