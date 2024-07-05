related_commands: open_document open_document_embedded_from_current_path

demo_code:
```python
s = sioyek.Sioyek('', launch_if_not_exists=True, launch_args=LAUNCH_ARGS)
s.goto_page_with_page_number(1)
start_recording(RECORDING_FILE_NAME)
show_run_command(s, 'open_document_embedded')
time.sleep(3)
end_recording()
```

for_commands:

doc_body:
Opens an embedded file browser in sioyek (unlike open_document which uses the operating system's file browser).