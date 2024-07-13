related_commands: keyboard_select_line

related_configs: ruler_display_mode

demo_code:
```python
s = sioyek.Sioyek('', launch_if_not_exists=True, launch_args=LAUNCH_ARGS)

s.open_document(PDF_FILES_PATH + '/attention.pdf')
s.toggleconfig_show_setconfig_in_statusbar()
s.setconfig_ruler_display_mode('slit')
s.goto_page_with_page_number(2)
s.keyboard_select_line('', wait=False)
time.sleep(0.3)
s.send_symbol('g')
s.send_symbol('a')
start_recording(RECORDING_FILE_NAME)
time.sleep(2)

values = [0, 3, 5, 10]
for val in values:
    s.setconfig_ruler_padding(f'{val}')
    time.sleep(1)

s.setconfig_ruler_padding('2')

values = [0, 3, 5, 10]
for val in values:
    s.setconfig_ruler_x_padding(f'{val}')
    time.sleep(1)


time.sleep(2)
end_recording()
```

for_configs: ruler_padding ruler_x_padding

doc_body:
Ruler padding when using `slit` ruler display mode.
