related_commands: keyboard_select_line

demo_code:
```python
s = sioyek.Sioyek('', launch_if_not_exists=True, launch_args=LAUNCH_ARGS)
s.goto_page_with_page_number(3)
s.keyboard_select_line('', wait=False)
time.sleep(1)
s.send_symbol('c')
s.send_symbol('a')
start_recording(RECORDING_FILE_NAME)
time.sleep(1)
show_run_command(s, 'move_ruler_down')
time.sleep(1)
s.move_visual_mark_down()
time.sleep(1)
s.move_visual_mark_down()
time.sleep(1)
s.move_visual_mark_up()
time.sleep(1)
s.move_visual_mark_up()
time.sleep(3)

end_recording()
```

for_commands: move_ruler_up move_ruler_down move_visual_mark_up move_visual_mark_down

doc_body:
Move the ruler up or down.