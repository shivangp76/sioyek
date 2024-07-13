related_commands: fit_to_page_width_ratio

related_configs:

demo_code:
```python
s = sioyek.Sioyek('', launch_if_not_exists=True, launch_args=LAUNCH_ARGS)
s.toggleconfig_show_setconfig_in_statusbar()
s.goto_page_with_page_number(1)


start_recording(RECORDING_FILE_NAME)
time.sleep(2)

ratios = [0.5, 0.75, 0.9]

for ratio in ratios:
    s.setconfig_fit_to_page_width_ratio(f'{ratio}')
    time.sleep(2)
    show_run_command(s, 'fit_to_page_width_ratio')
    time.sleep(2)

time.sleep(1)
end_recording()
```

for_configs: 

doc_body:
The screen width ratio when using @commnad(fit_to_page_width_ratio) command.
