related_commands: add_freetext_bookmark
related_configs:

type: bool

demo_code:
```python
s = sioyek.Sioyek('', launch_if_not_exists=True, launch_args=LAUNCH_ARGS)
s.toggleconfig_show_setconfig_in_statusbar()
s.goto_page_with_page_number(1)
s.toggleconfig_keyboard_point_selection()

s.add_freetext_bookmark(wait=False)
time.sleep(0.3)
s.send_symbol('a')
s.send_symbol('a')
s.send_symbol('l')
s.send_symbol('l')

type_words(s, 'This is a freetext bookmark with the default color', delay=0.02)
start_recording(RECORDING_FILE_NAME)
time.sleep(2)

for i in range(3):
    s.setconfig_render_freetext_borders('1')
    time.sleep(1)

    s.setconfig_render_freetext_borders('0')
    time.sleep(1)

time.sleep(3)
end_recording()
```

for_configs: 

doc_body:
Render the borders around freetext bookmarks.
