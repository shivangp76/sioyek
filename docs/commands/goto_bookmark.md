related_commands: add_bookmark add_marked_bookmark add_freetext_bookmark

demo_code:
```python
s = sioyek.Sioyek('', launch_if_not_exists=True, launch_args=LAUNCH_ARGS)
s.goto_page_with_page_number(1)
start_recording(RECORDING_FILE_NAME)
show_run_command(s, 'goto_bookmark')
time.sleep(1)
type_words(s, 'marked')
time.sleep(3)

end_recording()
```

for_commands: goto_bookmark goto_bookmark_g

doc_body:
@command(goto_bookmarke) opens a searchable list of current document's bookmarks. @command(goto_bookmark_g) opens a searchable list of all documents' bookmarks.