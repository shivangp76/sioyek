related_commands: portal overview_to_portal

demo_code:
```python
s = sioyek.Sioyek('', launch_if_not_exists=True, launch_args=LAUNCH_ARGS)
s.goto_page_with_page_number(1)
start_recording(RECORDING_FILE_NAME)
show_run_command(s, 'create_visible_portal')
time.sleep(1)
pause_recording()
s.escape()
s.toggleconfig_keyboard_point_selection()
s.create_visible_portal('cc')
resume_recording()
time.sleep(2)
s.goto_page_with_page_number(2)
show_run_command(s, 'portal')
time.sleep(2)
s.goto_page_with_page_number(1)
time.sleep(2)
s.overview_to_portal()
time.sleep(3)
end_recording()
```

for_commands:

doc_body:
Creates a portal with the selected window point as source. The source will be visible on the document. The destination of the portal can later be set using the @command(portal) command. An overview to the destination of the portal can be opened by right clicking on the portal icon.