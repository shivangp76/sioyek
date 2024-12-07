related_commands:
related_configs:

type: bool

demo_code:

for_configs: 

doc_body:
Normally we render entire pages as a whole. This can cause problems on high zoom levels on low-memory devices becasue a huge chunk of memory with the correct size might not be available. If @config(sliced_rendering) is set, instead of rendering pages as a whole, we slice the pages into multiple strips and render each strip.
