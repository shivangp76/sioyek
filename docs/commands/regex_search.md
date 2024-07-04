related_commands: search next_item previous_item overview_next_item overview_prev_item

demo_code:
```python
s = sioyek.Sioyek('', launch_if_not_exists=True, launch_args=LAUNCH_ARGS)
s.goto_page_with_page_number(1)
start_recording(RECORDING_FILE_NAME)
show_run_command(s, 'regex_search')
type_words(s, 's[a-z]{4}k')
for i in range(3):
    s.next_item()
    time.sleep(1)


end_recording()
```

doc_body:
Search the contents of current document using regular expressions. You can navigate the results the same way you would with @command(search) results.
```