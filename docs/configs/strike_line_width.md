related_commands: add_highlight

related_configs:

demo_code:
```python
s = sioyek.Sioyek('', launch_if_not_exists=True, launch_args=LAUNCH_ARGS)
s.toggleconfig_show_setconfig_in_statusbar()
s.goto_page_with_page_number(1)
s.search('fullscreen')
s.next_item()
s.select_current_search_match()
s.add_highlight('_')

start_recording(RECORDING_FILE_NAME)

for i in range(3):
    s.setconfig_strike_line_width('1')
    time.sleep(2)

    s.setconfig_strike_line_width('2')
    time.sleep(2)


time.sleep(1)
end_recording()
```

for_configs: 

doc_body:
The strike width when adding strike highlights (highlights with type `_`).
