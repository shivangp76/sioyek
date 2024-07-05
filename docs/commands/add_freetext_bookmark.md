related_commands: add_freetext_bookmark add_bookmark

demo_code:
```python
s = sioyek.Sioyek('', launch_if_not_exists=True, launch_args=LAUNCH_ARGS)
s.goto_page_with_page_number(1)
start_recording(RECORDING_FILE_NAME)
show_run_command(s, 'add_freetext_bookmark')
time.sleep(1)
s.escape()
s.toggleconfig_keyboard_point_selection()

s.add_freetext_bookmark('', wait=False)
pause_recording()
time.sleep(0.1)
s.send_symbol('c')
s.send_symbol('a')
s.send_symbol('h')
s.send_symbol('e')
resume_recording()
time.sleep(0.3)
type_words(s, 'This is a freetext bookmark', delay=0.03)

s.add_freetext_bookmark('', wait=False)
pause_recording()
time.sleep(0.1)
s.send_symbol('h')
s.send_symbol('a')
s.send_symbol('m')
s.send_symbol('e')
resume_recording()
time.sleep(0.3)
type_words(s, '#b', final_delay=1)

s.add_freetext_bookmark('', wait=False)
pause_recording()
time.sleep(0.1)
s.send_symbol('m')
s.send_symbol('a')
s.send_symbol('r')
s.send_symbol('e')
resume_recording()
time.sleep(0.3)
type_words(s, '#markdown This is a **markdown** <span style="color:red">bookmark</span>', delay=0.03)

s.add_freetext_bookmark('', wait=False)
pause_recording()
time.sleep(0.1)
s.send_symbol('a')
s.send_symbol('e')
s.send_symbol('e')
s.send_symbol('n')
resume_recording()
time.sleep(0.3)
type_words(s, '#latex x=5 \implies 2^x-1=31')
 
time.sleep(3)


s.goto_page_with_page_number(1)
time.sleep(1)
show_run_command(s, 'goto_bookmark')
time.sleep(1)
s.control_menu('select')
time.sleep(3)
end_recording()

```

for_commands:

doc_body:
Add a freetext bookmark in the selected rectangle. Freetext bookmarks are visible in the document. You can change the display style of the bookmark using the following prefixes:

- if the bookmark text starts with `#` followed by a symbol, it will draw a box with the symbol's color (the same color as the highlighs of the symbol).
- if the bookmark text starts with `#markdown`, the rest of the bookmark will be rendered as markdown
- if the bookmark text starts with `#latex`, the rest of the bookmark will be rendered as latex