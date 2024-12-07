related_commands: add_highlight change_highlight delete_highlight generic_delete
demo_code:
```python
s = sioyek.Sioyek('', launch_if_not_exists=True, launch_args=LAUNCH_ARGS)
s.goto_page_with_page_number(1)
start_recording(RECORDING_FILE_NAME)
show_run_command(s, 'add_annot_to_highlight')
time.sleep(3)
# we don't have a way to send symbols to sioyek
# so we cheat by cancelling the previous command
# and manually calling the command with the "selected" symbol
s.escape()
s.add_annot_to_highlight('a')
type_words(s, 'this is a new annotation for the selected highlight', delay=0.03)
time.sleep(1)
end_recording()
```

doc_body:
Allows the user to select a highlight to add annotations to using the keyboard. A label is displayed next to each visible highlight, and the user can press the corresponding key to select the highlight. Then a text box will appear where the user can type the annotation. 