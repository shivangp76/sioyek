related_commands: keyboard_select_line keyboard_overview keyboard_smart_jump

demo_code:
```python
s = sioyek.Sioyek('', launch_if_not_exists=True, launch_args=LAUNCH_ARGS)
time.sleep(1)
start_recording(RECORDING_FILE_NAME)
time.sleep(1)
show_run_command(s, 'keybord_select')
type_words(s, 'ga sa', delay=1, final_delay=2)

time.sleep(3)

end_recording()
```

for_commands: 

doc_body:
Displays a tag next to each word in the document, selects the text corresponding to the entered begin and end tag (separated by a space).