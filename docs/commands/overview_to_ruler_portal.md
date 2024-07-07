related_commands: create_visible_portal download_link download_overview_paper download_overview_paper_no_prompt

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
show_run_command(s, 'download_overview_paper_no_prompt')
time.sleep(7)
s.wait_for_downloads_to_finish('')
show_run_command(s, 'overview_to_ruler_portal')
time.sleep(3)
end_recording()
```

for_commands: overview_to_ruler_portal goto_ruler_portal

doc_body:
Open an overview/Jump to the portal on current ruler line.