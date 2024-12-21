related_commands: regex_search next_item previous_item overview_next_item overview_prev_item

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

s.escape()
s.goto_page_with_page_number(1)
s.search(wait=False)
time.sleep(0.3)
type_words(s, '<3,4>sioyek')
for i in range(3):
    s.next_item()
    time.sleep(1)

end_recording()
```
for_commands: search chapter_search

doc_body:
Search the contents of current document. You can use @command(next_item) and @command(previous_item) to navigate search results. You can optionally specify a page range using the following syntax: `<begin_page,end_page>query`. The @command(chapter_search) command automatically fills the page range with the current chapter's page range.