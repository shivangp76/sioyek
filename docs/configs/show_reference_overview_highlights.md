related_commands: overview_link overview_definition
related_configs:

type: bool

demo_code:
```python
s = sioyek.Sioyek('', launch_if_not_exists=True, launch_args=LAUNCH_ARGS)
s.open_document(PDF_FILES_PATH + '/attention.pdf')
s.wait_for_indexing_to_finish()
s.toggleconfig_show_setconfig_in_statusbar()
s.goto_page_with_page_number(2)
s.setconfig_show_reference_overview_highlights('1')
s.overview_link('a')
# s.focus_text('fullscreen')

start_recording(RECORDING_FILE_NAME)

time.sleep(2)

s.escape()
s.setconfig_show_reference_overview_highlights('0')
s.overview_link('a')

time.sleep(2)

end_recording()
```

for_configs: 

doc_body:
Highlight the reference paper name in overview window (this is the automatically detected paper name which will be downloaded with @command(download_overview_paper) command).
