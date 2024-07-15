related_commands: 

related_configs: 

for_configs:

demo_code:
```python
s = sioyek.Sioyek('', launch_if_not_exists=True, launch_args=LAUNCH_ARGS)
s.toggleconfig_show_setconfig_in_statusbar()
s.goto_page_with_page_number(1)

start_recording(RECORDING_FILE_NAME)

show_run_command(s, 'toggle_dark?')
time.sleep(2)
s.escape()
s.setconfig_documentation_font_size('20')
show_run_command(s, 'toggle_dark?')
time.sleep(2)

    

end_recording()
```

doc_body:
Font size of documentation.
