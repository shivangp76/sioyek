related_commands: command open_prev_doc goto_highlight goto_bookmark

related_configs: 
type: float

demo_code:
```python
s = sioyek.Sioyek('', launch_if_not_exists=True, launch_args=LAUNCH_ARGS)
s.toggleconfig_show_setconfig_in_statusbar()
s.goto_page_with_page_number(1)

start_recording(RECORDING_FILE_NAME)

s.command('', wait=False)
time.sleep(1)

values = [0.3, 0.5, 0.75, 1]
for val in values:
    s.escape()
    s.setconfig_menu_screen_width_ratio(f'{val}')
    s.command('', wait=False)
    time.sleep(1)

s.setconfig_menu_screen_width_ratio('0.75')

for val in values:
    s.escape()
    s.setconfig_menu_screen_height_ratio(f'{val}')
    s.command('', wait=False)
    time.sleep(1)

time.sleep(2)
end_recording()
```

for_configs: menu_screen_width_ratio menu_screen_height_ratio

doc_body:
The ratio of screen width/height to use for menus.
