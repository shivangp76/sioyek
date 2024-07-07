related_commands: search regex_search

demo_code:
```python
s = sioyek.Sioyek('', launch_if_not_exists=True)
start_recording(RECORDING_FILE_NAME)
s.search('', wait=False)
time.sleep(0.3)
# print('I am here')
# s.type_text('S')
type_words(s, 'Sioyek')
time.sleep(1)
show_run_command(s, 'overview_next_item')
time.sleep(1)
s.overview_next_item()
time.sleep(1)
s.overview_next_item()
time.sleep(1)
s.overview_next_item()
time.sleep(1)
s.overview_next_item()
time.sleep(2)
show_run_command(s, 'overview_prev_item')
time.sleep(1)
s.overview_prev_item()
time.sleep(1)
s.overview_prev_item()
time.sleep(1)
s.overview_prev_item()
time.sleep(3)
end_recording()
```
for_commands: overview_next_item overview_prev_item

doc_body:
Open an overview to the next/previous search result.