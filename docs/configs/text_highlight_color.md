related_commands: keyboard_select

demo_code:
```python
s = sioyek.Sioyek('', launch_if_not_exists=True, launch_args=LAUNCH_ARGS)
s.toggleconfig_show_setconfig_in_statusbar()
s.keyboard_select('ga sa')
time.sleep(1)

start_recording(RECORDING_FILE_NAME)
time.sleep(3)
s.setconfig_text_highlight_color('1 0 0')
time.sleep(1)
s.setconfig_text_highlight_color('1 0 1')
time.sleep(1)
s.setconfig_text_highlight_color('0 0 1')
time.sleep(3)

end_recording()
```

for_configs:

doc_body:
The highlight color of selected text (e.g. when user selects text using mouse or using commands like @command(keyboard_select)).