related_commands: overview_definition portal_to_definition visual_mark_under_cursor
demo_code:
```python
s = sioyek.Sioyek('', launch_if_not_exists=True)
s.goto_page_with_page_number(4)
start_recording(RECORDING_FILE_NAME)
s.focus_text('You can middle click on the name of figures')
time.sleep(1)
# this is not necessary, we only do it to highlight the target in the video
s.search('Figure 1')
time.sleep(0.5)
s.escape()
time.sleep(1)
show_run_command(s, 'goto_definition')
time.sleep(4)
s.prev_state()
time.sleep(1)
s.move_visual_mark_down()
time.sleep(1)
s.move_visual_mark_down()
time.sleep(2)
s.goto_definition()
time.sleep(4)
end_recording()
```

doc_body:
Go to the the definition of the first symbol in the highlighted line using the @concept(ruler).