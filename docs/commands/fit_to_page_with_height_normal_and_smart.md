demo_code:
```python
s = sioyek.Sioyek('', launch_if_not_exists=True, launch_args=LAUNCH_ARGS)
s.goto_page_with_page_number(1)
for i in range(3):
    s.zoom_out()
start_recording(RECORDING_FILE_NAME)
show_run_command(s, 'fit_to_page_width')
time.sleep(1)
show_run_command(s, 'fit_to_page_width_smart')
time.sleep(1)
show_run_command(s, 'fit_to_page_height')
time.sleep(1)
show_run_command(s, 'fit_to_page_height_smart')
time.sleep(2)
end_recording()
```

for_commands: fit_to_page_width fit_to_page_height fit_to_page_width_smart fit_to_page_height_smart fit_to_page_smart

doc_body:
Fits the document to page width/height. The smart variants try to ignore the white page margins while fitting. `fit_to_page_smart` command fits the document to the minimum of width and height, ignoring the white margins.