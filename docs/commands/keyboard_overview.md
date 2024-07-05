related_commands: keyboard_select_line keyboard_overview keyboard_smart_jump

demo_code:
```python
s = sioyek.Sioyek('', launch_if_not_exists=True, launch_args=LAUNCH_ARGS)
s.goto_page_with_page_number(4)
time.sleep(1)
start_recording(RECORDING_FILE_NAME)
time.sleep(1)
show_run_command(s, 'keyboard_overview')
type_words(s, 'sa', delay=1, final_delay=2)

time.sleep(3)

end_recording()
```

for_commands: 

doc_body:
Displays a tag next to each word in the document, opens an overview to the selected tag's word (this works even for documents that don't have links, otherwise @command(overview_link) would be used).