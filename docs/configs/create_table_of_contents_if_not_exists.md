related_commands: goto_toc

related_configs: 
type: bool

demo_code:
```python
s = sioyek.Sioyek('', launch_if_not_exists=True, launch_args=LAUNCH_ARGS)
s.toggleconfig_show_setconfig_in_statusbar()

start_recording(RECORDING_FILE_NAME)

s.setconfig_create_table_of_contents_if_not_exists('1')

show_run_command(s, 'goto_toc')
time.sleep(3)

end_recording()
```

for_configs: 

doc_body:
If the document doesn't have a table of contents, try to automatically generate one (this is a heuristic and doesn't work perfectly all the time).
