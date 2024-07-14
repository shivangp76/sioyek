related_commands:

related_configs:

demo_code:
```python
s = sioyek.Sioyek('', launch_if_not_exists=True, launch_args=LAUNCH_ARGS)
s.open_document(PDF_FILES_PATH + '/attention.pdf')
s.goto_page_with_page_number(2)
time.sleep(2)
start_recording(RECORDING_FILE_NAME)
time.sleep(1)

for i in range(3):
    s.setconfig_highlight_links('1')
    time.sleep(1)
    s.setconfig_highlight_links('0')
    time.sleep(1)
end_recording()
```

for_configs: 

doc_body:
Whether we should highlight PDF links.
