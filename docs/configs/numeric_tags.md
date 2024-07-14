related_commands: keyboard_select

related_configs:

demo_code:
```python
s = sioyek.Sioyek('', launch_if_not_exists=True, launch_args=LAUNCH_ARGS)
s.toggleconfig_show_setconfig_in_statusbar()

start_recording(RECORDING_FILE_NAME)

s.setconfig_numeric_tags('0')
time.sleep(1)
show_run_command(s, 'keyboard_select')
time.sleep(2)
s.escape()

s.setconfig_numeric_tags('1')
time.sleep(1)
show_run_command(s, 'keyboard_select')
time.sleep(2)

end_recording()
```

for_configs: 

doc_body:
When performing `keyboard_` commands that require a point to be selected, we associate a tag with each possible target. If this option is set, the tags only use numerical values instead of alphabetical values.
