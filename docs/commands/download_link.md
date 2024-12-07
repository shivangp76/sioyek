related_commands: open_link download_overview_paper

demo_code:
```python
s = sioyek.Sioyek('', launch_if_not_exists=True, launch_args=LAUNCH_ARGS)
s.open_document(PDF_FILES_PATH + '/attention.pdf')
s.goto_page_with_page_number(2)
start_recording(RECORDING_FILE_NAME)
time.sleep(2)
show_run_command(s, 'overview_link')
time.sleep(3)
s.send_symbol('c')
time.sleep(3)
s.close_overview()
time.sleep(1)
show_run_command(s, 'download_link')
time.sleep(3)
s.send_symbol('c')
time.sleep(7)
s.wait_for_downloads_to_finish('')
show_run_command(s, 'overview_portal')
time.sleep(3)
end_recording()
```

for_commands: 
requires_pro: true

doc_body:
Download a paper by selecting its link using the keyboard.