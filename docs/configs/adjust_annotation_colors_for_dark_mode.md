related_commands: add_highlight add_freetext_bookmark

related_configs:
type: bool

demo_code:
```python
s = sioyek.Sioyek('', launch_if_not_exists=True, launch_args=LAUNCH_ARGS)
s.toggleconfig_show_setconfig_in_statusbar()
s.goto_page_with_page_number(1)
s.search('fullscreen')
s.next_item()
s.select_current_search_match()
s.add_highlight('y')

s.toggle_dark_mode()
start_recording(RECORDING_FILE_NAME)

for i in range(3):
    s.setconfig_adjust_annotation_colors_for_dark_mode('0')
    time.sleep(2)

    s.setconfig_adjust_annotation_colors_for_dark_mode('1')
    time.sleep(2)

time.sleep(1)
end_recording()
```

for_configs: 

doc_body:
Automatically adjust annotation colors to be more appropriate in dark/custom color modes.
