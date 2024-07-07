related_commands: search regex_search semantic_search semantic_search_extractive

demo_code:
```python
s = sioyek.Sioyek('', launch_if_not_exists=True, launch_args=LAUNCH_ARGS)
s.toggleconfig_show_setconfig_in_statusbar()
s.goto_page_with_page_number(1)
start_recording(RECORDING_FILE_NAME)
show_run_command(s, 'search')
type_words(s, 'sioyek')

s.next_item()
time.sleep(3)

s.setconfig_search_highlight_color('1 0 0')
time.sleep(1)
s.setconfig_search_highlight_color('1 0 1')
time.sleep(1)
s.setconfig_search_highlight_color('0 0 1')
time.sleep(3)

end_recording()
```

for_configs: 

doc_body:
Highligh color of search matches.