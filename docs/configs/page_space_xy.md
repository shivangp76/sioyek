related_commands: toggle_two_page_mode
type: int

related_configs: 

demo_code:
```python
s = sioyek.Sioyek('', launch_if_not_exists=True, launch_args=LAUNCH_ARGS)
s.toggleconfig_show_setconfig_in_statusbar()
s.goto_page_with_page_number(1)
s.toggle_custom_color()
s.goto_bottom_of_page()
s.screen_down()

start_recording(RECORDING_FILE_NAME)
time.sleep(1)

show_run_command(s, 'toggle_two_page_mode')
s.fit_to_page_width()
time.sleep(1)

values = [0, 10, 100]

for val in values:
    s.setconfig_page_space_y(f'{val}')
    time.sleep(1)

for val in values:
    s.setconfig_page_space_x(f'{val}')
    time.sleep(1)

time.sleep(2)

end_recording()
```

for_configs: page_space_x page_space_y

doc_body:
Spacing between pages in two page mode.
