related_commands: ruler_under_cursor toggle_ruler_scroll_mode

related_configs:

demo_code:
```python
s = sioyek.Sioyek('', launch_if_not_exists=True, launch_args=LAUNCH_ARGS)
s.goto_page_with_page_number(1)
s.focus_text('fullscreen')
s.toggleconfig_show_setconfig_in_statusbar()


start_recording(RECORDING_FILE_NAME)

s.setconfig_autocenter_ruler('1')


end_recording()
```

for_configs: 

doc_body:
Always center the document on the ruler when moving the ruler.
