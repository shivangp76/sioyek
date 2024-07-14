related_commands: search

related_configs: unselected_search_highlight_color

demo_code:
```python
s = sioyek.Sioyek('', launch_if_not_exists=True, launch_args=LAUNCH_ARGS)
s.toggleconfig_show_setconfig_in_statusbar()
s.goto_page_with_page_number(1)
s.search('the')
start_recording(RECORDING_FILE_NAME)

s.next_item()

for i in range(3):
    s.setconfig_should_highlight_unselected_search('1')
    time.sleep(1)
    s.setconfig_should_highlight_unselected_search('0')
    time.sleep(1)
end_recording()
```

for_configs: 

doc_body:
Whether we should highlight the found search results that are not the current search result.
