related_commands: overview_definition download_link download_paper_under_cursor
related_configs: papers_folder_path paper_download_should_create_portal

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
show_run_command(s, 'download_overview_paper')
time.sleep(2)
s.control_menu('select')
time.sleep(3)
s.wait_for_downloads_to_finish('')
time.sleep(2)
s.overview_to_portal()
time.sleep(3)
s.close_overview()
s.delete_portal()
s.focus_text('transduction problems')
s.overview_definition()
time.sleep(2)
show_run_command(s, 'download_overview_paper_no_prompt')
time.sleep(3)
s.wait_for_downloads_to_finish('')
time.sleep(3)
end_recording()
```

for_commands: download_overview_paper download_overview_paper_no_prompt
requires_pro: true

doc_body:
Downloads the paper currently shown in overview window. The @command(download_overview_paper) command first displays the automatically detected paper title and asks for confirmation. The @command(download_overview_paper_no_prompt) command downloads the paper without asking for confirmation.