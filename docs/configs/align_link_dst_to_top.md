related_commands: 

related_configs: 

demo_code:
```python
s = sioyek.Sioyek('', launch_if_not_exists=True, launch_args=LAUNCH_ARGS)
s.open_document(PDF_FILES_PATH + '/attention.pdf')
s.goto_page_with_page_number(2)
s.toggleconfig_show_setconfig_in_statusbar()


s.set_status_string('We will open link "a"')
start_recording(RECORDING_FILE_NAME)

s.open_link('', wait=False)
time.sleep(1)
s.send_symbol('a')
time.sleep(2)
s.prev_state()

s.setconfig_align_link_dest_to_top('1')
s.open_link('', wait=False)
time.sleep(1)
s.send_symbol('a')


time.sleep(3)
end_recording()
```

for_configs: 

doc_body:
Normally when jumping to links, we align the middle of the screen with the link destination. If this option is set we align the top of the screen instead.
