related_commands: toggle_synctex synctex_under_cursor synctex_under_ruler

related_configs:
type: string

demo_code:

for_configs: 

doc_body:
The command line program to open the corresponding file when peforming a synctex inverse search. `%1` will expand to the path of TeX file and `%2` will exapnd to the line number.

For example here is how to configure it for vscode on windows:
```
inverse_search_command      "C:\path\to\Code.exe" -r -g "%1":%2
```
