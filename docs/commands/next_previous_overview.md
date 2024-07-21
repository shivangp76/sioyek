related_commands: overview_definition

demo_code:
```python
s = sioyek.Sioyek('', launch_if_not_exists=True, launch_args=LAUNCH_ARGS)
s.open_document(PDF_FILES_PATH + '/attention.pdf')
s.goto_page_with_page_number(2)
s.focus_text('transduction problems')
start_recording(RECORDING_FILE_NAME)
time.sleep(2)
show_run_command(s, 'overview_definition')
time.sleep(2)
show_run_command(s, 'next_overview')
time.sleep(2)
s.next_overview()
time.sleep(1)
s.previous_overview()
time.sleep(1)
s.previous_overview()
time.sleep(3)
end_recording()
```

for_commands: next_overview previous_overview

doc_body:
Go to the next/prevoius possible overview desitnation. For example, when executing @command(overview_definition) there might be more than one possible target. This command will jump to the next/previous target.