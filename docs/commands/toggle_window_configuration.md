related_commands: portal

demo_code:
```python
s = sioyek.Sioyek('', launch_if_not_exists=True, launch_args=LAUNCH_ARGS)
s.goto_page_with_page_number(1)
s.portal()
s.goto_page_with_page_number(2)
s.portal()
s.goto_page_with_page_number(1)
# s.keyboard_select_line('', wait=False)
time.sleep(1)
start_recording(RECORDING_FILE_NAME)
time.sleep(1)
show_run_command(s, 'toggle_window_configuration')
time.sleep(2)
show_run_command(s, 'toggle_window_configuration')
time.sleep(3)

end_recording()
```

for_commands:

doc_body:
Toggle helper window. The helper window displays the portal destination of the closest portal source to current location.