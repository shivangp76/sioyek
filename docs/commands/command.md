related_commands: command_palette

demo_code:

for_commands:

doc_body:
Open a searchable list of all commands. Appending or prepending a `?` to the search query will open the documentation for the selected command. If the query starts with `=` it will show a list of `setconfig` commands which are used to dynamically set configurations, changes made using `setconfig` are not permenant. If you want to permenantly set a config, you can enter `+=` which will search for `setsaveconfig` commands which persistantly set configurations. Alternatively if you want to save the current value of a configuration you can enter `+` which will search the `saveconfig` commands which saves the current value of the selected configuration. If the query starts with `!` it will show a list of `toggleconfig` commands which are used to toggle the value of boolean configurations.