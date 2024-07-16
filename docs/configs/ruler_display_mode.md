related_commands: ruler_under_cursor

related_configs:

for_configs: 

demo_code:
```python
s = sioyek.Sioyek('', launch_if_not_exists=True, launch_args=LAUNCH_ARGS)
s.toggleconfig_show_setconfig_in_statusbar()
s.goto_page_with_page_number(1)
s.focus_text('fullscreen')

start_recording(RECORDING_FILE_NAME)
time.sleep(1)

values = ['box', 'slit', 'underline', 'highlight_below']
for val in values:
    s.setconfig_ruler_display_mode(f'{val}')
    time.sleep(2)


end_recording()
```

doc_body:
Specifies how the ruler should be rendered. The possible values are:

- `box`: draw a box around selected line
- `slit`: draw a shadow on everything except the selected line
- `underline`: draw a line under the selected line
- `highlight_below`: highlight the entire screen area below the line
