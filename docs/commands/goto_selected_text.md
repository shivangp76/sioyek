related_commands:

demo_code:
```python

s = sioyek.Sioyek('', launch_if_not_exists=True, launch_args=LAUNCH_ARGS)
time.sleep(1)
s.keyboard_select('ga sa')
# time.sleep(0.3)
# type_words(s, 'ga sa', delay=1, final_delay=2)
start_recording(RECORDING_FILE_NAME)
time.sleep(1)

for i in range(100):
    s.move_down()
    time.sleep(0.01)

show_run_command(s, 'goto_selected_text')
time.sleep(3)

end_recording()

```

for_commands:

doc_body:
Jump to the location of selected text.