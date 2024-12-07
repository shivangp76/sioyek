related_commands: keyboard_select_line ruler_under_cursor
related_configs: ruler_display_mode

type: color4

demo_code:
```python
s = sioyek.Sioyek('', launch_if_not_exists=True, launch_args=LAUNCH_ARGS)
s.toggleconfig_show_setconfig_in_statusbar()
s.goto_page_with_page_number(1)
s.goto_bottom_of_page()
s.screen_down()
s.focus_text('fullscreen')
s.setconfig_ruler_display_mode('slit')

start_recording(RECORDING_FILE_NAME)
time.sleep(1)

colors = ['0 0 0 0.3', '0 0 0 0.5', '0 0 1 0.1']
for color in colors:
    s.setconfig_ruler_slit_color(color)
    time.sleep(1)

time.sleep(2)

end_recording()
```

for_configs: ruler_slit_color

doc_body:
Ruler color when @config(ruler_display_mode) is set to `slit`.