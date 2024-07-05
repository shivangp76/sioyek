related_commands: 

demo_code:
```python
s = sioyek.Sioyek('', launch_if_not_exists=True, launch_args=LAUNCH_ARGS)
s.goto_page_with_page_number(1)
start_recording(RECORDING_FILE_NAME)
s.edit_visible_bookmark('a', wait=False)
time.sleep(3)
s.open_external_text_editor()
time.sleep(10)

end_recording()
```

for_commands:

doc_body:
Open an external text editor to edit the current text input. For example you can open an external editor to edit bookmarks or highlight annotations. The external editor can be configured using @config(external_text_editor_command), the corresponding text input will be updated in sioyek whenever the file is saved.