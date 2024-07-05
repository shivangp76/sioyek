related_commands: toggle_freehand_drawing_mode toggle_pen_drawing_mode

demo_code:
```python
s = sioyek.Sioyek('', launch_if_not_exists=True, launch_args=LAUNCH_ARGS)
s.goto_page_with_page_number(1)
start_recording(RECORDING_FILE_NAME)
time.sleep(2)
show_run_command(s, 'clear_current_page_drawings')
time.sleep(2)
end_recording()
```

for_commands: clear_current_page_drawings clear_current_document_drawings

doc_body:
Clear all freehand drawings in the current page/document.