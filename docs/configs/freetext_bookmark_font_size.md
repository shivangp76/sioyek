related_commands: add_freetext_bookmark

related_configs: freetext_bookmark_color
type: int

demo_code:
```python
s = sioyek.Sioyek('', launch_if_not_exists=True, launch_args=LAUNCH_ARGS)
s.toggleconfig_show_setconfig_in_statusbar()
s.goto_page_with_page_number(1)
s.toggleconfig_keyboard_point_selection()

start_recording(RECORDING_FILE_NAME)
time.sleep(2)

s.add_freetext_bookmark(wait=False)
time.sleep(0.3)
s.send_symbol('a')
s.send_symbol('a')
s.send_symbol('l')
s.send_symbol('l')

type_words(s, 'This is a freetext bookmark with the default font size', delay=0.02)
time.sleep(2)
s.delete_bookmark()

sizes = [5, 10, 20]

for size in sizes:
    s.setconfig_freetext_bookmark_font_size(f'{size}')
    s.add_freetext_bookmark(wait=False)
    time.sleep(0.3)
    s.send_symbol('a')
    s.send_symbol('a')
    s.send_symbol('l')
    s.send_symbol('l')

    type_words(s, f'This is a freetext bookmark with font size="{size}"', delay=0.02)
    time.sleep(2)
    s.delete_bookmark()

time.sleep(3)
end_recording()
```

for_configs: 

doc_body:
Font size to use when adding new freetext bookmarks.
