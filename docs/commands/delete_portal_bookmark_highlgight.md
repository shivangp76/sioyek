related_commands: 

demo_code:
```python
s = sioyek.Sioyek('', launch_if_not_exists=True, launch_args=LAUNCH_ARGS)
s.goto_page_with_page_number(1)
start_recording(RECORDING_FILE_NAME)
show_run_command(s, 'goto_end')
time.sleep(1)
show_run_command(s, 'prev_state')
time.sleep(2)
show_run_command(s, 'next_state')
time.sleep(2)
end_recording()
```

for_commands: delete_portal delete_bookmark delete_highlight

doc_body:
Delete the selected portal/bookmark/highlight. If no item is selected then we delete the closest portal/bookmark/highlight to the current location.