related_commands:
related_configs:

type: bool

demo_code:
```python
s = sioyek.Sioyek('', launch_if_not_exists=True, launch_args=LAUNCH_ARGS)
s.open_document(PDF_FILES_PATH + '/attention.pdf')
s.goto_page_with_page_number(1)
s.toggleconfig_show_setconfig_in_statusbar()


start_recording(RECORDING_FILE_NAME)

s.setconfig_toc_jump_align_top('0')
time.sleep(1)
s.goto_toc('', wait=False)
time.sleep(0.3)
type_words(s, 'introduction')
time.sleep(1)
s.control_menu('select')
time.sleep(2)

s.setconfig_toc_jump_align_top('1')
time.sleep(1)
s.goto_toc('', wait=False)
time.sleep(0.3)
type_words(s, 'introduction')
time.sleep(1)
s.control_menu('select')
time.sleep(2)


end_recording()
```

for_configs: 

doc_body:
When jumping to a table of contents entry, align the top of the screen with the target location (instead of center of the screen).
