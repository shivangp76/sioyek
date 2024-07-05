related_commands: 

demo_code:
```python
s = sioyek.Sioyek('', launch_if_not_exists=True, launch_args=LAUNCH_ARGS)
s.open_document(PDF_FILES_PATH + '/attention.pdf')
s.goto_page_with_page_number(1)
start_recording(RECORDING_FILE_NAME)
time.sleep(2)
show_run_command(s, 'next_chapter')
time.sleep(3)
end_recording()
```

for_commands: next_chapter prev_chapter

doc_body:
Go to the next/previous chapter in table of contents.