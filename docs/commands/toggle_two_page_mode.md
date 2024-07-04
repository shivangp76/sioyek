demo_code:
```python
s = sioyek.Sioyek('', launch_if_not_exists=True, launch_args=LAUNCH_ARGS)
s.goto_page_with_page_number(1)
start_recording(RECORDING_FILE_NAME)
show_run_command(s, 'toggle_two_page_mode')
time.sleep(3)
end_recording()

```
doc_body:
Toggles between two-page mode and single-page mode.