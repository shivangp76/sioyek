related_commands: search

related_configs:
type: bool

demo_code:
```python
s = sioyek.Sioyek('', launch_if_not_exists=True, launch_args=LAUNCH_ARGS)
s.toggleconfig_show_setconfig_in_statusbar()
s.setconfig_should_highlight_unselected_search('1')
s.setconfig_incremental_search(1)
s.goto_page_with_page_number(1)
s.wait_for_indexing_to_finish('')

start_recording(RECORDING_FILE_NAME)

s.search('', wait=False)
time.sleep(1)

type_words(s, 'sioyek', delay=0.5)

end_recording()
```

for_configs: 

doc_body:
If this is set, then we perform searches as user is typing the search query.
