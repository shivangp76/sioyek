related_commands: keyboard_select_line move_ruler_up move_ruler_down

demo_code:
```python
s = sioyek.Sioyek('', launch_if_not_exists=True, launch_args=LAUNCH_ARGS)
s.goto_page_with_page_number(3)
s.keyboard_select_line(wait=False)
time.sleep(1)
s.send_symbol('p')
s.send_symbol('a')
start_recording(RECORDING_FILE_NAME)
time.sleep(1)
show_run_command(s, 'move_ruler_to_next_block')
time.sleep(1)
s.move_ruler_to_next_block()
time.sleep(1)
s.move_ruler_to_prev_block()
time.sleep(1)
s.move_ruler_to_prev_block()
time.sleep(3)

end_recording()
```

for_commands: move_ruler_to_next_block move_ruler_to_prev_block

doc_body:
Move the ruler to previous/next text block.