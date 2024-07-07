related_commands: keyboard_select_line move_ruler_up move_ruler_down

demo_code:
```python
s = sioyek.Sioyek('', launch_if_not_exists=True, launch_args=LAUNCH_ARGS)
s.goto_page_with_page_number(3)
for i in range(3):
    s.zoom_in()

s.goto_left_smart()

s.keyboard_select_line('', wait=False)
time.sleep(1)
s.send_symbol('c')
s.send_symbol('a')
start_recording(RECORDING_FILE_NAME)
time.sleep(1)
show_run_command(s, 'move_ruler_next')
time.sleep(1)

for i in range(10):
    s.move_visual_mark_next()
    time.sleep(1)

time.sleep(3)

end_recording()
```

for_commands: move_ruler_next move_ruler_prev move_visual_mark_next move_visual_mark_prev

doc_body:
Move the ruler such that more content is visible. This is the same as @command(move_ruler_down) @command(move_ruler_up) commands when the ruler is entirely visible, but when part of the ruler is offscreen (e.g. in mobile devices with a small screen) this command moves such that the obscured part of the ruler becomes visible and only moves to the next line when the entire line has been viewed.
A red marked highlights the leftmost part of the next ruler position, so when you reach the red mark you can run @command(move_ruler_next) and continue reading from the left side of the screen.