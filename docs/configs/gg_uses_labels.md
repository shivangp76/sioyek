related_commands: goto_beginning

related_configs:
type: bool

demo_code:

for_configs: 

doc_body:
If set, then goto_beginning command uses page labels instead of the absolute page number when using `<num>gg` command. For example pressing `2gg` normally jumps to the second page of the document, but in some documents there are page labels like "i", "ii", "iii", etc. So the page labeled 2 might not actually be the second page of the document. If this config is set, then `2gg` will jump to the page labeled "2" instead of the second page of the document.