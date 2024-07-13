related_commands:

related_configs: 

demo_code:
```python
s = sioyek.Sioyek('', launch_if_not_exists=True, launch_args=LAUNCH_ARGS)
s.toggleconfig_show_setconfig_in_statusbar()

start_recording(RECORDING_FILE_NAME)

show_run_command(s, 'toggleconfig_keyboard_point_selection')
show_run_command(s, 'add_freetext_bookmark')
time.sleep(3)

s.send_symbol('a')
time.sleep(1)
s.send_symbol('a')
time.sleep(1)

s.send_symbol('e')
time.sleep(1)
s.send_symbol('e')
time.sleep(1)

time.sleep(3)

end_recording()
```

for_configs: 

doc_body:
If set, when executing commands that require a point on the screen to be selected, we show a list of labels on the screen and allow the user to select the point by typing the label using keyboard.
