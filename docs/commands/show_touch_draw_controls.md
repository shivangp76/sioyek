related_commands: show_touch_main_menu

demo_code:
```python
s = sioyek.Sioyek('', launch_if_not_exists=True, launch_args=LAUNCH_ARGS)
s.goto_page_with_page_number(3)
start_recording(RECORDING_FILE_NAME)
show_run_command(s, 'show_touch_draw_controls')
time.sleep(3)
end_recording()
```

for_commands:

doc_body:
Show the freehand drawing menu for touch devices.