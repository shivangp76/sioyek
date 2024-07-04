related_commands: zoom_in zoom_out

demo_code:
```python
s = sioyek.Sioyek('', launch_if_not_exists=True, launch_args=LAUNCH_ARGS)
s.goto_page_with_page_number(4)
s.focus_text('You can middle click')
s.overview_definition()
start_recording(RECORDING_FILE_NAME)
s.zoom_out_overview()
time.sleep(1)
s.zoom_out_overview()
time.sleep(1)
s.zoom_in_overview()
time.sleep(1)
s.zoom_in_overview()
time.sleep(2)
end_recording()

```

for_commands: zoom_in_overview zoom_out_overview

doc_body:
Zoom in/out in the overview window.