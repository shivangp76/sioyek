related_commands: 
related_configs: search_url_[a-z]

demo_code:
```python
s = sioyek.Sioyek('', launch_if_not_exists=True, launch_args=LAUNCH_ARGS)
s.goto_page_with_page_number(1)
s.search('Sioyek')
s.next_item()
s.select_current_search_match()
start_recording(RECORDING_FILE_NAME)
time.sleep(2)
show_run_command(s, 'external_search')
time.sleep(2)
s.send_symbol('g')
time.sleep(3)
end_recording()
```

for_commands:

doc_body:
Search the selected text using the configured external search engine. This is a symbol command, and each symbol could be configured for a different search engine using @config(search_url_[a-z]). For example in the default configuration `g` searches in google and `s` searches in google scholar. If no text is selected, we search the title of the current document instead (e.g. in the default configuration, to search the current paper in google scholar you can press `ss` when no text is selected).