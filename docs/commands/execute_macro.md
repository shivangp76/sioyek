demo_code:
```python
s = sioyek.Sioyek('', launch_if_not_exists=True, launch_args=LAUNCH_ARGS)
s.goto_page_with_page_number(1)
start_recording(RECORDING_FILE_NAME)
show_run_command(s, 'execute_macro')
type_words(s, "toggle_dark_mode;set_status_string('hello from macro')", delay=0.03, final_delay=3)
time.sleep(3)
end_recording()

```

doc_body:
Execute the given sioyek commands string. This is mainly used for scripting and it is probably not directly useful for end-users.