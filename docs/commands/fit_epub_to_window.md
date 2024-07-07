related_commands:

demo_code:
```python
s = sioyek.Sioyek('', launch_if_not_exists=True, launch_args=LAUNCH_ARGS)
s.open_document(PDF_FILES_PATH + '/epub.epub')
s.goto_page_with_page_number(20)
time.sleep(2)
start_recording(RECORDING_FILE_NAME)

for i in range(3):
    s.zoom_in()
    time.sleep(0.5)

show_run_command(s, 'fit_epub_to_window')
time.sleep(5)
end_recording()
```

for_commands:

doc_body:
Fit an epub document exactly to the window dimensions.