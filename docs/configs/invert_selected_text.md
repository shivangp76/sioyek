related_commands:

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

start_recording(RECORDING_FILE_NAME)

time.sleep(2)

highlight_styles = ['transparent', 'inverted', 'background']
for style in highlight_styles:
    s.setconfig_selected_text_highlight_style(style)
    time.sleep(2)

    # s.setconfig_invert_selected_text('0')
    # time.sleep(2)


time.sleep(1)
end_recording()
```

for_configs: 

doc_body:
Invert the selected text to highlight it.
