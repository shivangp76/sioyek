related_commands: open_external_text_editor

related_configs: 

demo_code:

doc_body:
The command line program to use when running @command(open_external_text_editor) command. In this config, `%{file}` expands to the file that should be opened. For example:

```
external_text_editor_command nvim "%{file}"
```
