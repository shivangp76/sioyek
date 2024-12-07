related_commands: search

related_configs: 
type: bool

demo_code:
```python
s = sioyek.Sioyek('', launch_if_not_exists=True, launch_args=LAUNCH_ARGS)
s.toggleconfig_show_setconfig_in_statusbar()
s.goto_page_with_page_number(1)
s.search('fullscreen')
s.next_item()
s.select_current_search_match()

s.setconfig_fill_textbar_with_selected_text('1')

start_recording(RECORDING_FILE_NAME)

time.sleep(1)

show_run_command(s, 'search')

time.sleep(3)
end_recording()
```

for_configs: 

doc_body:
When opening the textbar (e.g. when executing @command(search) or any other command that requires text input) automatically fill the input with the selected text.
