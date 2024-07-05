related_commands: open_link

demo_code:
```python
s = sioyek.Sioyek('', launch_if_not_exists=True, launch_args=LAUNCH_ARGS)
s.open_document(PDF_FILES_PATH + '/attention.pdf')
s.goto_page_with_page_number(2)
start_recording(RECORDING_FILE_NAME)
time.sleep(2)
show_run_command(s, 'overview_link')
time.sleep(3)
s.send_symbol('a')
time.sleep(3)
end_recording()
```

for_commands: 

doc_body:
Displays a symbol next to each visible link and opens an overview to link destination when user enters the link symbol.