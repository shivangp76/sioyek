related_commands:

related_configs: 

for_configs: shift_click_command control_click_command commnad_click_command hold_middle_click_command tablet_pen_click_commnad tablet_pen_double_click_command right_click_command middle_click_command shift_middle_click_command control_middle_click_command command_middle_click_command alt_middle_click_command shift_right_click_command control_right_click_command command_right_click_command alt_click_command alt_right_click_command

demo_code:

doc_body:
@concept(macro) commands to run when the corresponding button is clicked (while possibly holding modifier keys). Example:

```
alt_click_command toggle_dark_mode;toggle_scrollbar;set_status_string('hi there')
```

