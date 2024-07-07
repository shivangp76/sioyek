related_commands: search regex_search

demo_code:
```python
s = sioyek.Sioyek('', launch_if_not_exists=True, launch_args=LAUNCH_ARGS)
s.goto_page_with_page_number(1)
start_recording(RECORDING_FILE_NAME)
show_run_command(s, 'search')
type_words(s, 'sioyek')
for i in range(3):
    s.next_item()
    time.sleep(1)

show_run_command(s, 'select_current_search_match')
time.sleep(3)

end_recording()
```
for_commands:

doc_body:
Selects the current highlighted search result.