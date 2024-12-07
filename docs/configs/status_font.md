related_commands: 
related_configs:
type: string

demo_code:
```python
s = sioyek.Sioyek('', launch_if_not_exists=True, launch_args=LAUNCH_ARGS)
s.toggleconfig_show_setconfig_in_statusbar()
s.goto_page_with_page_number(1)

start_recording(RECORDING_FILE_NAME)
time.sleep(2)

s.setconfig_status_font('Times New Roman')
time.sleep(2)

s.setconfig_status_font('Consolas')
time.sleep(2)

end_recording()
```

for_configs: 

doc_body:
Font face to use for menus.
