related_commands: add_freetext_bookmark add_marked_bookmark goto_bookmark goto_bookmark_g

demo_code:
```python
s = sioyek.Sioyek('', launch_if_not_exists=True, launch_args=LAUNCH_ARGS)
s.goto_page_with_page_number(2)
start_recording(RECORDING_FILE_NAME)
show_run_command(s, 'add_bookmark')
type_words(s, 'This is a bookmark')
s.goto_page_with_page_number(1)
time.sleep(1)
show_run_command(s, 'goto_bookmark')
time.sleep(1)
s.control_menu('select')
time.sleep(3)
end_recording()

```

for_commands:

doc_body:
Add an invisible bookmark in the current location. You can later search the bookmarks using the @command(goto_bookmark) command.