related_commands: add_freetext_bookmark
related_configs: 

type: bool

demo_code:

for_configs: 

doc_body:
If this option is set, we always render freetext bookmarks. Normally, we render freetext bookmarks in another thread to avoid blocking the main thread. This means that sometimes freetext bookmarks are not rendered immediately. If you want to always render freetext bookmarks immediately, you can set this option, but it might caues performance degradation.