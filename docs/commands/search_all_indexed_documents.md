related_commands: create_fulltext_index_for_current_document

demo_code:
```python
s = sioyek.Sioyek('', launch_if_not_exists=True, launch_args=LAUNCH_ARGS)
s.goto_page_with_page_number(3)
s.create_fulltext_index_for_current_document()
time.sleep(1)
start_recording(RECORDING_FILE_NAME)
show_run_command(s, 'search_all_indexed_documents')
time.sleep(0.3)
type_words(s, 'sioyek commands', final_delay=3)
time.sleep(2)
end_recording()
```

for_commands:

doc_body:
Perform a fulltext search in all documents indexed by @command(create_fulltext_index_for_current_document) or @config(automatically_index_documents_for_fulltext_search).