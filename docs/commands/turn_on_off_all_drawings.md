related_commands: toggle_freehand_drawing_mode toggle_pen_drawing_mode set_freehand_type toggle_drawing_mask

demo_code:
```python
s = sioyek.Sioyek('', launch_if_not_exists=True, launch_args=LAUNCH_ARGS)
s.goto_begining()
start_recording(RECORDING_FILE_NAME)
show_run_command(s, 'turn_off_all_drawings')
time.sleep(2)
show_run_command(s, 'turn_on_all_drawings')
time.sleep(3)

end_recording()
```
for_commands: turn_on_all_drawings turn_off_all_drawings

doc_body:
Turn all drawings on/off.