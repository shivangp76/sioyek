related_commands: add_highlight generic_select

demo_code:
```python
s = sioyek.Sioyek('', launch_if_not_exists=True, launch_args=LAUNCH_ARGS)
s.load_annotations_file()
s.wait_for_renders_to_finish('')
s.goto_page_with_page_number(1)
start_recording(RECORDING_FILE_NAME)
s.generic_select('a')
time.sleep(1)
show_run_command(s, 'edit_selected_highlight')
time.sleep(0.3)
type_words(s, ' This is a new edit')
time.sleep(2)
end_recording()
```

doc_body:
Edits the annotation of selected highlight (highlights can be selected by clicking on them).