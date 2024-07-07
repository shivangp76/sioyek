related_commands: keyboard_select_line

demo_code:
```python
s = sioyek.Sioyek('', launch_if_not_exists=True, launch_args=LAUNCH_ARGS)
s.open_document(PDF_FILES_PATH + '/attention.pdf')
s.toggleconfig_show_setconfig_in_statusbar()
s.goto_page_with_page_number(2)
s.keyboard_select_line(['g', 'a'])
start_recording(RECORDING_FILE_NAME)
time.sleep(2)

s.setconfig_ruler_color('1 0 0')
time.sleep(1)
s.setconfig_ruler_color('1 0 1')
time.sleep(1)
s.setconfig_ruler_color('0 0 1')

time.sleep(3)
end_recording()
```

for_configs: 

doc_body:
Ruler color.