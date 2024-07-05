related_commands: add_freetext_bookmark add_bookmark

demo_code:
```python
s = sioyek.Sioyek('', launch_if_not_exists=True, launch_args=LAUNCH_ARGS)
s.goto_page_with_page_number(3)
start_recording(RECORDING_FILE_NAME)
show_run_command(s, 'add_marked_bookmark')
time.sleep(1)
s.escape()
s.toggleconfig_keyboard_point_selection()
s.add_marked_bookmark('cc', wait=False)
time.sleep(0.3)
type_words(s, 'This is a marked bookmark')
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
Add a marked bookmark in the selected location. Marked bookmarks are visible in the document using a small icon. You can later search the bookmarks using the @command(goto_bookmark) command.