related_commands: 

related_configs: 

demo_code:
```python
s = sioyek.Sioyek('', launch_if_not_exists=True, launch_args=LAUNCH_ARGS)
s.open_document(PDF_FILES_PATH + '/epub.epub')
s.goto_page_with_page_number(20)
time.sleep(2)
s.toggleconfig_show_setconfig_in_statusbar()

start_recording(RECORDING_FILE_NAME)

s.setconfig_epub_width(1000)
s.fit_to_page_width()
time.sleep(3)

s.setconfig_epub_width(500)
s.fit_to_page_width()
time.sleep(3)

s.setconfig_epub_font_size(18)

time.sleep(3)

end_recording()
```

for_configs: epub_width epub_height epub_font_size

doc_body:
Configure the appearance of epub documents.
