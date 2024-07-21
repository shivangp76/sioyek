related_commands: synctex_under_cursor synctex_under_ruler toggle_synctex
related_configs: use_ruler_to_highlight_synctex_line synctex_highlight_timeout synctex_highlight_timeout

demo_code:
```python
s = sioyek.Sioyek('', launch_if_not_exists=True, launch_args=LAUNCH_ARGS)
s.toggleconfig_show_setconfig_in_statusbar()
s.open_document(str(tutorial_file_path))
s.goto_page_with_page_number(3)
s.setconfig_use_ruler_to_highlight_synctex_line('0')
s.setconfig_synctex_highlight_timeout('20')
start_recording(RECORDING_FILE_NAME)

s.synctex_forward_search([str(tutorial_tex_path).replace('\\', '/'), '170'])

time.sleep(3)

s.setconfig_synctex_highlight_color('1 0 0')
time.sleep(1)
s.setconfig_synctex_highlight_color('1 0 1')
time.sleep(1)
s.setconfig_synctex_highlight_color('0 0 1')
time.sleep(3)

end_recording()
```

for_configs: 

doc_body:
Highlight color to use when highlighting synctex search results. This only applies when @config(use_ruler_to_highlight_synctex_line) is set to 0, otherwise we use the ruler to highlight the line.
