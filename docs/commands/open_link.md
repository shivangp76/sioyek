related_commands: overview_link

demo_code:
```python
s = sioyek.Sioyek('', launch_if_not_exists=True, launch_args=LAUNCH_ARGS)
s.open_document(PDF_FILES_PATH + '/attention.pdf')
s.goto_page_with_page_number(2)
start_recording(RECORDING_FILE_NAME)
time.sleep(2)
show_run_command(s, 'open_link')
time.sleep(3)
s.send_symbol('a')
time.sleep(3)
end_recording()
```

for_commands: 

doc_body:
Opens PDF links using keyboard. Displays a symbol next to each visible link and jumps when user enters the link symbol.