related_commands:

demo_code:
```python
s = sioyek.Sioyek('', launch_if_not_exists=True, launch_args=LAUNCH_ARGS)
s.goto_page_with_page_number(1)
s.screen_down()
start_recording(RECORDING_FILE_NAME)
show_run_command(s, 'goto_top_of_page')
time.sleep(2)
show_run_command(s, 'goto_bottom_of_page')
time.sleep(3)
end_recording()
```

for_commands: goto_top_of_page goto_bottom_of_page

doc_body:
Go to the top/bottom of the current page.