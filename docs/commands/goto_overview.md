related_commands: overview_definition

demo_code:
```python
s = sioyek.Sioyek('', launch_if_not_exists=True, launch_args=LAUNCH_ARGS)
s.open_document(PDF_FILES_PATH + '/attention.pdf')
s.goto_page_with_page_number(2)
s.focus_text('transduction problems')
start_recording(RECORDING_FILE_NAME)
time.sleep(2)
s.overview_definition()
time.sleep(2)
show_run_command(s, 'goto_overview')
# s.goto_overview()
time.sleep(3)
end_recording()

```

for_commands:

doc_body:
Jump to the location of current overview.