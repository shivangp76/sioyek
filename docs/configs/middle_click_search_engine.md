related_commands: external_search

related_configs: search_url_[a-z]
type: symbol

demo_code:

for_configs: middle_click_search_engine shift_middle_click_search_engine

doc_body:
When middle clicking on a paper name, we will search for it in the engine (as configured in @config(search_url_[a-z])) set here.

For example:
```
search_url_s https://scholar.google.com/scholar?q=
middle_click_search_engine s
```
means that we will search the paper name in google scholar when user middle clicks on a paper name.
