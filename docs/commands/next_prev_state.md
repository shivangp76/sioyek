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

for_commands: next_state prev_state

doc_body:
Move forward/backward in history. Note that this history works even across files (i.e. if you go to a different file you can return to the previous file by executing @command(prev_state) or go back forward to the new file by executing @command(next_state).)