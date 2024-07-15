related_commands: 

related_configs: 

for_configs:

demo_code:
```python
s = sioyek.Sioyek('', launch_if_not_exists=True, launch_args=LAUNCH_ARGS)
s.toggleconfig_show_setconfig_in_statusbar()
s.goto_page_with_page_number(1)

start_recording(RECORDING_FILE_NAME)

vals = [10, 15, 20]

for val in vals:
    s.setconfig_status_bar_font_size(f'{val}')
    time.sleep(1)

end_recording()
```

doc_body:
Font size of the status bar.
