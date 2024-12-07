related_commands: open_link toggle_highlight_links
type: color3

dlemo_code:
```python
s = sioyek.Sioyek('', launch_if_not_exists=True, launch_args=LAUNCH_ARGS)
s.toggleconfig_show_setconfig_in_statusbar()
s.open_document(PDF_FILES_PATH + '/attention.pdf')
s.goto_page_with_page_number(2)
start_recording(RECORDING_FILE_NAME)
s.toggle_highlight_links()

time.sleep(3)

s.setconfig_link_highlight_color('1 0 0')
time.sleep(1)
s.setconfig_link_highlight_color('1 0 1')
time.sleep(1)
s.setconfig_link_highlight_color('0 0 1')
time.sleep(3)

end_recording()
```

for_configs: 

doc_body:
Highlight color to use when highlighting links.