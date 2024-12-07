related_commands: overview_to_portal toggle_window_configuration

demo_code:
```python
s = sioyek.Sioyek('', launch_if_not_exists=True, launch_args=LAUNCH_ARGS)
s.goto_page_with_page_number(1)
start_recording(RECORDING_FILE_NAME)
show_run_command(s, 'portal')
time.sleep(1)
s.goto_page_with_page_number(2)
time.sleep(1)
show_run_command(s, 'portal')
time.sleep(1)
s.goto_page_with_page_number(1)
time.sleep(1)
show_run_command(s, 'overview_to_portal')
time.sleep(2)
s.close_overview()
show_run_command(s, 'toggle_window_configuration')
time.sleep(3)
end_recording()

```

for_commands:

doc_body:
Creates a new pending portal with the current location as source. If we already have a pending-portal, create the portal with the current location as the destination. As an example use-case you can create a portal from a part of text describing an equation or table to the equation/table itself, the contents of the destination portal can be viewed in a separate window using @command(toggle_window_configuration) or an overview to see the contents of the portal in the same window using @command(overview_to_portal).

If this command is executed while an @concept(overview) is visible, then it creates a "pinned portal" from the current location to the overview. Pinned portals are portals that are displayed in a box like overviews so their destination content is always visible.