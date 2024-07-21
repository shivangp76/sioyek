related_commands: 

demo_code:
```python
s = sioyek.Sioyek('', launch_if_not_exists=True, launch_args=LAUNCH_ARGS)
path = PDF_FILES_PATH + '/attention.pdf'
# s.goto_page_with_page_number(2)
start_recording(RECORDING_FILE_NAME)

time.sleep(2)
s.open_document(path)
time.sleep(2)
show_run_command(s, 'open_last_document')
time.sleep(3)
end_recording()
```

for_commands: 

doc_body:
Opens the last document that was opened in the current sioyek window.