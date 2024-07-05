related_commands: add_highlight

demo_code:
```python
s = sioyek.Sioyek('', launch_if_not_exists=True, launch_args=LAUNCH_ARGS)
s.goto_page_with_page_number(3)
start_recording(RECORDING_FILE_NAME)
show_run_command(s, 'goto_highlight')
time.sleep(2)
type_words(s, 'dark')
time.sleep(2)
show_run_command(s, 'goto_highlight')
time.sleep(2)
type_words(s, 'b ', select=False)
time.sleep(2)
type_words(s, 'fit')
time.sleep(3)

end_recording()

```

for_commands: goto_highlight goto_highlight_g

doc_body:
Open a searchable list of all highlights in the current document. You can limit the search to highlights of a certain type by prefixing the query by the type of the highlight followed by a space. For example `j {query}` searches for `{query}` only in highlights of type j. @command(goto_highlight_g) opens a searchable list of all documents' highlights.