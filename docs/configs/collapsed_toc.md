
related_commands: goto_toc

related_configs: 
type: bool

demo_code:
```python
s = sioyek.Sioyek('', launch_if_not_exists=True, launch_args=LAUNCH_ARGS)
s.open_document(PDF_FILES_PATH + '/attention.pdf')
s.goto_page_with_page_number(2)
s.toggleconfig_show_setconfig_in_statusbar()

s.setconfig_collapsed_toc('0')
time.sleep(1)

start_recording(RECORDING_FILE_NAME)

s.goto_toc('', wait=False)
time.sleep(2)
s.escape()

show_run_command(s, 'toggleconfig_collapsed_toc')
s.goto_toc('', wait=False)

time.sleep(3)
end_recording()
```

for_configs: 

doc_body:
Open the table of contents in collapsed state by default.
