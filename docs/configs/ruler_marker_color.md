related_commands: keyboard_select_line move_ruler_next move_ruler_prev

demo_code:
```python
s = sioyek.Sioyek('', launch_if_not_exists=True, launch_args=LAUNCH_ARGS)
s.toggleconfig_show_setconfig_in_statusbar()
s.goto_page_with_page_number(3)
for i in range(3):
    s.zoom_in()

s.goto_left_smart()

s.keyboard_select_line(['c', 'a'])
# time.sleep(1)
for i in range(8):
    s.move_visual_mark_next()

start_recording(RECORDING_FILE_NAME)
time.sleep(1)

s.setconfig_ruler_marker_color('1 1 0')
time.sleep(1)
s.setconfig_ruler_marker_color('1 0 1')
time.sleep(1)
s.setconfig_ruler_marker_color('0 0 1')
time.sleep(3)

end_recording()
```

for_configs: 

doc_body:
The ruler marker color to highlight the next left side of the screen when using @command(move_ruler_next).