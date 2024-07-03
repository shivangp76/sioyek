related_commands: toggle_freehand_drawing_mode toggle_pen_drawing_mode set_freehand_type turn_on_all_drawings turn_off_all_drawings

demo_code:
```python
s = sioyek.Sioyek('', launch_if_not_exists=True, launch_args=LAUNCH_ARGS)
s.goto_begining()
start_recording(RECORDING_FILE_NAME)
time.sleep(1)
symbols = ['r', 'b', 'c', 'd']

for sym in symbols:
    s.toggle_drawing_mask(sym)
    time.sleep(1)
    s.toggle_drawing_mask(sym)
    time.sleep(1)

end_recording()
```

doc_body:
This is a symbol command which given a symbol `s`, toggles the visibility of freehand drawings with type `s`.