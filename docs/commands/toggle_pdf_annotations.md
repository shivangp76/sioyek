related_commands: 

demo_code:
```python
s = sioyek.Sioyek('', launch_if_not_exists=True, launch_args=LAUNCH_ARGS)
s.open_document(PDF_FILES_PATH + '/tutorial_with_annotations.pdf')
s.goto_page_with_page_number(1)
start_recording(RECORDING_FILE_NAME)
time.sleep(2)
show_run_command(s, 'toggle_pdf_annotations')
time.sleep(3)
end_recording()
```

for_commands: 

doc_body:
Toggles the visibility of PDF annotations (these are different from sioyek annotations, the PDF annotations are embedded in the PDF file itself).