related_commands: screen_down screen_up

related_configs:
type: float

demo_code:
```python
s = sioyek.Sioyek('', launch_if_not_exists=True, launch_args=LAUNCH_ARGS)
s.toggleconfig_show_setconfig_in_statusbar()
s.goto_page_with_page_number(1)
s.toggleconfig_keyboard_point_selection()

start_recording(RECORDING_FILE_NAME)

s.setconfig_move_screen_ratio('1')
time.sleep(2)
show_run_command(s, 'screen_down')
time.sleep(2)

s.goto_page_with_page_number(1)
s.setconfig_move_screen_ratio('0.5')
time.sleep(2)
show_run_command(s, 'screen_down')
time.sleep(2)

s.goto_page_with_page_number(1)
s.setconfig_move_screen_ratio('0.25')
time.sleep(2)
show_run_command(s, 'screen_down')
time.sleep(2)


time.sleep(1)
end_recording()
```

for_configs: move_screen_ratio move_screen_percentage

doc_body:
The ratio of screen height to move when executing @command(screen_down) and @command(screen_up).
