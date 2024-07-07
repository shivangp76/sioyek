related_commands: close_window new_window

demo_code:
```python
s = sioyek.Sioyek('', launch_if_not_exists=True, launch_args=LAUNCH_ARGS)
s.goto_page_with_page_number(1)
start_recording(RECORDING_FILE_NAME)
show_run_command(s, 'new_window')
time.sleep(1)
s.open_document(PDF_FILES_PATH + '/attention.pdf')
time.sleep(1)
show_run_command(s, 'goto_window')
time.sleep(2)
type_words(s, 'tut')

time.sleep(3)
end_recording()
```

for_commands:

doc_body:
Open a searchable list of current sioyek windows.