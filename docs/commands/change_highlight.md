
related_commands: add_highlight add_annot_to_highlight

demo_code:
```python
s = sioyek.Sioyek('', launch_if_not_exists=True, launch_args=LAUNCH_ARGS)
s.goto_page_with_page_number(1)
start_recording(RECORDING_FILE_NAME)
show_run_command(s, 'change_highlight')
time.sleep(3)
s.send_symbol('a')
time.sleep(1)
s.send_symbol('r')
time.sleep(3)
end_recording()
```

doc_body:
Alows the user to change highlight type using keyboard. The user first enters the label of the highlight they want to change, then they enter the label of the new highlight type they want to change it to.