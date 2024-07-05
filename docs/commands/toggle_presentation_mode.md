related_commands: 

demo_code:
```python
s = sioyek.Sioyek('', launch_if_not_exists=True, launch_args=LAUNCH_ARGS)
s.goto_page_with_page_number(1)
start_recording(RECORDING_FILE_NAME)
time.sleep(2)
show_run_command(s, 'toggle_presentation_mode')
time.sleep(2)
show_run_command(s, 'next_page')
time.sleep(3)
end_recording()
```

for_commands: toggle_presentation_mode turn_on_presentation_mode

doc_body:
Toggle/turn on presentation mode, in this mode we fit each slide to the entire page and movement commands move to the next/previous slide.