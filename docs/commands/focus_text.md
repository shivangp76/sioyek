related_commands:

demo_code:
```python

s = sioyek.Sioyek('', launch_if_not_exists=True, launch_args=LAUNCH_ARGS)
time.sleep(1)
start_recording(RECORDING_FILE_NAME)
time.sleep(1)


show_run_command(s, 'focus_text')
time.sleep(1)
type_words(s, 'fullscreen')
time.sleep(3)

end_recording()

```

for_commands:

doc_body:
Create the ruler under the line with the given text.