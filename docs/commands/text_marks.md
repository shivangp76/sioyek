related_commands: keyboard_select select_ruler_text
demo_code:
```python
s = sioyek.Sioyek('', launch_if_not_exists=True)
s.goto_page_with_page_number(3)
s.keyboard_select('ba xb')
start_recording(RECORDING_FILE_NAME)
show_run_command(s, 'toggle_text_mark')
time.sleep(1)
s.toggle_text_mark()
time.sleep(1)
show_run_command(s, 'move_text_mark_forward')
for i in range(10):
    s.move_text_mark_forward()
    time.sleep(0.1)
s.toggle_text_mark()
time.sleep(1)
for i in range(10):
    s.move_text_mark_forward()
    time.sleep(0.1)

show_run_command(s, 'move_text_mark_backward')

for i in range(10):
    s.move_text_mark_backward()
    time.sleep(0.1)
s.toggle_text_mark()
time.sleep(1)
for i in range(10):
    s.move_text_mark_backward()
    time.sleep(0.1)

show_run_command(s, 'move_text_mark_forward_word')
for i in range(4):
    s.move_text_mark_forward_word()
    time.sleep(0.4)
s.toggle_text_mark()
time.sleep(1)
for i in range(4):
    s.move_text_mark_forward_word()
    time.sleep(0.4)

show_run_command(s, 'move_text_mark_backward_word')
for i in range(4):
    s.move_text_mark_backward_word()
    time.sleep(0.4)
s.toggle_text_mark()
time.sleep(1)
for i in range(4):
    s.move_text_mark_backward_word()
    time.sleep(0.4)
time.sleep(2)
s.toggle_text_mark()
show_run_command(s, 'move_text_mark_down')
time.sleep(1)
s.move_text_mark_down()
show_run_command(s, 'move_text_mark_up')
time.sleep(1)
s.move_text_mark_up()
time.sleep(2)

end_recording()
```
for_commands: toggle_text_mark move_text_mark_forward move_text_mark_backward move_text_mark_forward_word move_text_mark_backward_word move_text_mark_down move_text_mark_up

doc_body:
These commands are used to manipulate the text selection using keyboard. `move_{forward/backward}` commands move the cursor one character forward/backward and `move_{forward/backward}_word` commands move the cursor one word forward/backward, and `move_{down/up}` commands move the cursor one line down/up. `toggle_text_mark` toggles the position of cursor between the start and end of text selection.