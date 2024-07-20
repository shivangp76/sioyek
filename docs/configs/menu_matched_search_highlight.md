related_commands: 

related_configs: 

demo_code:
```python
s = sioyek.Sioyek('', launch_if_not_exists=True, launch_args=LAUNCH_ARGS)
s.goto_page_with_page_number(1)
s.toggleconfig_show_setconfig_in_statusbar()

start_recording(RECORDING_FILE_NAME)

s.command('', wait=False)
time.sleep(0.3)
type_words(s, 'toggle dark', select=False)
time.sleep(3)

s.setconfig_menu_matched_search_highlight('text-decoration: underline;')
s.escape()

s.command('', wait=False)
time.sleep(0.3)
type_words(s, 'toggle dark', select=False)
time.sleep(3)

end_recording()
```

for_configs:

doc_body:
The CSS style used to highlight matched search results in menus.
