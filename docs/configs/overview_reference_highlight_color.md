related_commands: overview_definition download_overview_paper
type: color3

demo_code:
```python
s = sioyek.Sioyek('', launch_if_not_exists=True, launch_args=LAUNCH_ARGS)
s.toggleconfig_show_setconfig_in_statusbar()
s.open_document(PDF_FILES_PATH + '/attention.pdf')
s.goto_page_with_page_number(2)
s.focus_text('transduction problems')
start_recording(RECORDING_FILE_NAME)
time.sleep(2)
s.overview_definition()
time.sleep(2)

s.setconfig_overview_reference_highlight_color('1 0 0')
time.sleep(1)
s.setconfig_overview_reference_highlight_color('1 0 1')
time.sleep(1)

s.setconfig_overview_reference_highlight_color('0 0 1')

time.sleep(3)
end_recording()
```

for_configs: 

doc_body:
Highlight color to use when highlighting the automatically detected overview paper name.