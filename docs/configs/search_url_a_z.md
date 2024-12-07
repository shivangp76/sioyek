related_commands: 
related_configs:

type: string

demo_code:

for_configs: search_url_[a-z]

doc_body:
Search url for @command(external_search) command. After executing external search, the user can enter the symbol corresponding to the configured search engine to perform the search using that engine.

For example if we have the following in `prefs_user.config`:

```
search_url_g https://www.google.com/search?q=
search_url_s https://scholar.google.com/scholar?q=
```

Then executing @command(external_search) followed by `s` will search google and if it is followed by `s` it will search google scholar.
