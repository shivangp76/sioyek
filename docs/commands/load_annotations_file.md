related_commands: write_annotations_file

demo_code:

for_commands: load_annotations_file import_annotations_file_sync_deleted

doc_body:
Load the annotations from the file created using @command(write_annotations_file). The `load_annotations_file` doesn't delete anything from the current file but the `import_annotations_file_sync_deleted` will overwrite the current annotations with the annotations in the file and delete any annotations that are not in the file.