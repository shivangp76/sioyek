related_commands: generic_select

demo_code:
```python
s = sioyek.Sioyek('', launch_if_not_exists=True, launch_args=LAUNCH_ARGS)
s.goto_page_with_page_number(1)
start_recording(RECORDING_FILE_NAME)
show_run_command(s, 'generic_delete')
time.sleep(3)
s.send_symbol('a')
time.sleep(3)
end_recording()

```

doc_body:
Allows the user to delete annotations using keyboard. The user enters the label of the annotation they want to delete.