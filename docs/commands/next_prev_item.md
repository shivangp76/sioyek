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
show_run_command(s, 'next_item')
time.sleep(1)
s.next_item()
time.sleep(1)
s.next_item()
time.sleep(1)
s.next_item()
time.sleep(1)
s.next_item()
time.sleep(2)
show_run_command(s, 'previous_item')
time.sleep(1)
s.previous_item()
time.sleep(1)
s.previous_item()
time.sleep(1)
s.previous_item()
time.sleep(3)
end_recording()
```
for_commands: next_item previous_item

doc_body:
Go to the next/previous search result.