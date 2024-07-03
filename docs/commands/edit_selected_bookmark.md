related_commands: add_bookmark add_freetext_bookmark add_marked_bookmark

demo_code:
```python
s = sioyek.Sioyek('', launch_if_not_exists=True, launch_args=LAUNCH_ARGS)
s.load_annotations_file()
s.wait_for_renders_to_finish('')
start_recording(RECORDING_FILE_NAME)
s.generic_select('a')
time.sleep(1)
show_run_command(s, 'edit_selected_bookmark')
time.sleep(0.3)
type_words(s, ' This is a new edit')
time.sleep(2)
end_recording()
```

doc_body:
Edits the selected bookmark (a bookmark can be selected by clicking on it).