related_commands: fit_to_page_width

demo_code:
```python
s = sioyek.Sioyek('', launch_if_not_exists=True, launch_args=LAUNCH_ARGS)
s.goto_page_with_page_number(1)
start_recording(RECORDING_FILE_NAME)
show_run_command(s, 'fit_to_page_width_ratio')
time.sleep(3)
end_recording()
```

for_commands:

doc_body:
Fit to current document to a ratio of screen configured using @config(fit_to_page_width_ratio). This is useful for users with very wide screens.