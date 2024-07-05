related_commands: keys keys_user keys_user_all prefs_user prefs_user_all

demo_code:
```python
s = sioyek.Sioyek('', launch_if_not_exists=True, launch_args=LAUNCH_ARGS)
s.goto_page_with_page_number(1)
start_recording(RECORDING_FILE_NAME)
time.sleep(1)
show_run_command(s, 'prefs')

time.sleep(3)

end_recording()
```

for_commands: 

doc_body:
Opens the default `prefs.config` file.