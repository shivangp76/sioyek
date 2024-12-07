related_commands: toggle_presentation_mode

related_configs: 
type: bool

demo_code:
```python
s = sioyek.Sioyek('', launch_if_not_exists=True, launch_args=LAUNCH_ARGS)
s.toggleconfig_show_setconfig_in_statusbar()
s.toggle_presentation_mode()

start_recording(RECORDING_FILE_NAME)

s.setconfig_ignore_whitespace_in_presentation_mode('0')
time.sleep(2)

s.setconfig_ignore_whitespace_in_presentation_mode('1')
time.sleep(2)


end_recording()
```

for_configs: 

doc_body:
Ignore whitespace when fitting pages to screen in presentation mode.
