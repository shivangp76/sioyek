related_commands: add_freetext_bookmark

demo_code:
```python
s = sioyek.Sioyek('', launch_if_not_exists=True, launch_args=LAUNCH_ARGS)
s.goto_page_with_page_number(1)
s.generic_select('a')
s.edit_selected_bookmark(wait=False)
start_recording(RECORDING_FILE_NAME)
time.sleep(2)
s.increase_freetext_font_size()
time.sleep(1)
s.increase_freetext_font_size()
time.sleep(1)
s.increase_freetext_font_size()
time.sleep(1)
s.decrease_freetext_font_size()
time.sleep(1)
s.decrease_freetext_font_size()
time.sleep(1)
s.decrease_freetext_font_size()
time.sleep(1)

end_recording()
```

for_commands: increase_freetext_font_size decrease_freetext_font_size

doc_body:
Increase/decrease the font size of the freetext bookmark being edited.