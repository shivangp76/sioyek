related_commands: 

related_configs: 

demo_code:
```python
s = sioyek.Sioyek('', launch_if_not_exists=True, launch_args=LAUNCH_ARGS)
s.toggleconfig_show_setconfig_in_statusbar()
s.goto_page_with_page_number(1)
s.screen_down()
s.zoom_in()
s.zoom_in()
s.zoom_in()

start_recording(RECORDING_FILE_NAME)

values = [0.1, 0.5, 0.7, 1, 2, 3]

for gamma in values:
    s.setconfig_gamma(f'{gamma}')
    time.sleep(1)


time.sleep(2)
end_recording()
```

for_configs: 

doc_body:
The gamma value, each pixel value `p` will be replace with `p^gamma`. Higher gamma values make fonts look thicker and lower gamma values make them look thinner.
