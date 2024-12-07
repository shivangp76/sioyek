related_commands: search regex_search semantic_search semantic_search_extractive
related_configs: should_highlight_unselected_search
type: color3

demo_code:
```python
s = sioyek.Sioyek('', launch_if_not_exists=True, launch_args=LAUNCH_ARGS)
s.toggleconfig_show_setconfig_in_statusbar()
s.setconfig_should_highlight_unselected_search('1')
s.goto_page_with_page_number(1)
s.search('the')
start_recording(RECORDING_FILE_NAME)

s.next_item()
time.sleep(3)

s.setconfig_unselected_search_highlight_color('1 0 0')
time.sleep(1)
s.setconfig_unselected_search_highlight_color('1 0 1')
time.sleep(1)
s.setconfig_unselected_search_highlight_color('0 0 1')
time.sleep(3)

end_recording()
```

for_configs: 

doc_body:
The highlight color to highlight unselected search matches when @config(should_highlight_unselected_search) is set.