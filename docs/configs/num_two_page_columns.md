related_commands: toggle_two_page_mode
related_configs: 

type: int

demo_code:
```python
s = sioyek.Sioyek('', launch_if_not_exists=True, launch_args=LAUNCH_ARGS)
s.toggleconfig_show_setconfig_in_statusbar()
s.goto_page_with_page_number(1)
start_recording(RECORDING_FILE_NAME)
show_run_command(s, 'toggle_two_page_mode')
time.sleep(2)
s.setconfig_num_two_page_columns('3')
s.fit_to_page_width()
time.sleep(3)
end_recording()

```
doc_body:
Changes the number of page columns in "two" page mode.