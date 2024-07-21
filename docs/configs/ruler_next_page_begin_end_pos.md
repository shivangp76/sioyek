related_commands: move_ruler_down move_ruler_up toggle_ruler_scroll_mode

related_configs:

demo_code:
```python
s = sioyek.Sioyek('', launch_if_not_exists=True, launch_args=LAUNCH_ARGS)
s.toggleconfig_show_setconfig_in_statusbar()
s.goto_page_with_page_number(1)

def setup(s):
    s.goto_page_with_page_number(1)
    s.focus_text('fullscreen')

start_recording(RECORDING_FILE_NAME)

setup(s)
for i in range(20):
    s.move_ruler_down()
    time.sleep(0.1)


show_run_command(s, 'toggleconfig_visualize_ruler_thresholds')
time.sleep(2)
# s.toggleconfig_visualize_ruler_thresholds()

for i in range(20):
    s.move_ruler_down()
    time.sleep(0.1)

setup(s)
show_run_command(s, 'setconfig_ruler_next_page_begin_pos')
time.sleep(0.1)
type_words(s, '0.5')
# s.setconfig_ruler_next_page_begin_pos(0.5)
time.sleep(2)

for i in range(20):
    s.move_ruler_down()
    time.sleep(0.1)


# s.toggleconfig_visualize_ruler_thresholds()

for i in range(20):
    s.move_ruler_down()
    time.sleep(0.1)

position_values = [0, 0.25, 0.5, 0.75, 1]
for val in position_values:
    s.setconfig_ruler_next_page_begin_pos(f'{val}')
    time.sleep(1)

for val in position_values:
    s.setconfig_ruler_next_page_end_pos(f'{val}')
    time.sleep(1)

time.sleep(1)
end_recording()
```

for_configs: ruler_next_page_end_pos ruler_next_page_begin_pos

doc_body:
@config(ruler_next_page_end_pos) determines when we should move to the next page when using the ruler and when we do so, @config(ruler_next_page_begin_pos) determines where the new location of the ruler should be in the screen.

@config(ruler_next_page_begin_pos) is relative to the center of the screen and @config(ruler_next_page_end_pos) is relative to the bottom of the screen. For example, @config(ruler_next_page_begin_pos)=1 means we move one unit (each unit is half screen height) up from the middle of the screen which means we move to the top of the screen when moving to the next page.
