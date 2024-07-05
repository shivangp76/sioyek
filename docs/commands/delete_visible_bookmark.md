related_commands: add_marked_bookmark add_freetext_bookmark edit_visible_bookmark

demo_code:
```python
s = sioyek.Sioyek('', launch_if_not_exists=True, launch_args=LAUNCH_ARGS)
s.goto_page_with_page_number(1)
start_recording(RECORDING_FILE_NAME)
show_run_command(s, 'delete_visible_bookmark')
time.sleep(2)
s.send_symbol('a')
time.sleep(3)
end_recording()
```

for_commands:

doc_body:
Delete a visible bookmark on the screen using keyboard by specifying the label of the bookmark.