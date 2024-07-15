related_commands: open_link keyboard_select

related_configs: 

for_configs:

demo_code:
```python
s = sioyek.Sioyek('', launch_if_not_exists=True, launch_args=LAUNCH_ARGS)
s.open_document(PDF_FILES_PATH + '/attention.pdf')
s.toggleconfig_show_setconfig_in_statusbar()
s.goto_page_with_page_number(2)
# s.focus_text('fullscreen')

start_recording(RECORDING_FILE_NAME)

s.open_link('', wait=False)
time.sleep(2)

s.setconfig_tag_font_face('Times New Roman')
time.sleep(3)

end_recording()

```

doc_body:
Font face to use for keyboard tags.
