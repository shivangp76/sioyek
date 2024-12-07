related_commands:
related_configs: page_separator_width

type: bool

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

values = [0, 1, 5, 10, 20]


for val in values:
    s.setconfig_page_separator_width(f'{val}')
    time.sleep(1)

show_run_command(s, 'toggleconfig_real_page_separation')
s.setconfig_page_separator_width('0')
time.sleep(1)


for val in values:
    s.setconfig_page_space_y(f'{val}')
    time.sleep(1)


time.sleep(2)

end_recording()
```

for_configs: 

doc_body:
The page separator configured using @config(page_separator_width) doesn't actually separate the pages, it is simply a box drawn to distinguish the pages. If @config(real_page_separation) is set, then we actually move the pages to separate them (technically it is less efficient than the other method which is why it is not the default but it shouldn't matter in most cases).
Also note that when this option is set, the distance between pages is not configured with @config(page_separator_width) rather it uses @config(page_space_y).
