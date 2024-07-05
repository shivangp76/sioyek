related_commands:

demo_code:
```python
s = sioyek.Sioyek('', launch_if_not_exists=True, launch_args=LAUNCH_ARGS)
s.open_document(PDF_FILES_PATH + '/attention.pdf')
s.goto_page_with_page_number(2)
time.sleep(2)
start_recording(RECORDING_FILE_NAME)
show_run_command(s, 'toggle_highlight_links')
time.sleep(3)
end_recording()
```

for_commands:

doc_body:
Highlight the PDF links.