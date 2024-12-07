related_commands:

related_configs:
type: bool

demo_code:

for_configs: 

doc_body:
If set, when dragging the document (e.g. in touch mode) we ignore very small horizontal scrolls because when the user is trying to scroll vertically they probably will also do some horizontal scrollling inadvertently. So we start the dragging in a "snapped" state (meaning we are snapped to the original horizontal position) and only unspan when the user moves horizontally past a threshold.
