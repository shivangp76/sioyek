related_commands:

related_configs: 

demo_code:
```python
la = {key: val for key, val in LAUNCH_ARGS.items()}
la['--file-path'] = PDF_FILES_PATH + '/attention.pdf'
s = sioyek.Sioyek('', launch_if_not_exists=True, launch_args=la)
s.load_annotations_file()
# s.load
# time.sleep(3)
# s.reload()
s.toggleconfig_show_setconfig_in_statusbar()
s.goto_page_with_page_number(1)
s.screen_down()


start_recording(RECORDING_FILE_NAME)
time.sleep(1)

for color in LIGHT_COLORS:
    s.setconfig_question_bookmark_text_color(color)
    time.sleep(1)

for color in DARK_COLORS:
    s.setconfig_question_bookmark_background_color(color)
    time.sleep(1)


time.sleep(2)

end_recording()
```

for_configs: question_bookmark_background_color question_bookmark_text_color

doc_body:
The background and text color of question bookmarks (the bookmarks starting with `? `, see @command(add_freetext_bookmark) for more information).