related_commands: toggle_synctex

related_configs: 

demo_code:
```python
s = sioyek.Sioyek('', launch_if_not_exists=True, launch_args=LAUNCH_ARGS)
s.toggleconfig_show_setconfig_in_statusbar()
s.open_document(str(tutorial_file_path))
s.goto_page_with_page_number(3)
s.setconfig_use_ruler_to_highlight_synctex_line('0')

start_recording(RECORDING_FILE_NAME)

s.synctex_forward_search([str(tutorial_tex_path).replace('\\', '/'), '170'])
time.sleep(2)

show_run_command(s, 'setconfig_use_ruler_to_highlight_synctex_line')
time.sleep(0.3)
type_words(s, '1')
# s.setconfig_use_ruler_to_highlight_synctex_line('1')
s.synctex_forward_search([str(tutorial_tex_path).replace('\\', '/'), '170'])

time.sleep(3)


end_recording()
```

for_configs: 

doc_body:
Use the ruler to highlight the synctex search result (instead of highlighting it with a color).
