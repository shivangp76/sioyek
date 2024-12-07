related_commands: ruler_under_cursor

demo_code:
```python
s = sioyek.Sioyek('', launch_if_not_exists=True, launch_args=LAUNCH_ARGS)
s.open_document(PDF_FILES_PATH + '/attention.pdf')
s.goto_page_with_page_number(2)
start_recording(RECORDING_FILE_NAME)
time.sleep(2)
show_run_command(s, 'keyboard_select_line')
time.sleep(2)
s.send_symbol('g')
time.sleep(1)
s.send_symbol('a')
time.sleep(3)
end_recording()
```

for_commands: 

doc_body:
Select a line using keyboard. Show a label next to each visible line and create a ruler under that line when a label corresponding to that line is entered.