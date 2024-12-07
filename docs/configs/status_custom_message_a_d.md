related_commands:
related_configs: status_bar_format right_status_bar_format status_custom_message_[a-d]_command

type: string

for_configs: status_custom_message_[a-d] status_custom_message_[a-d]_command

demo_code:

doc_body:
Show custom messages which can be shown in statusbar if `%{custom_message_a}` to `%{custom_message_d}` is added to @config(status_bar_format). When the message is clicked, the command configured in @config(status_custom_message_[a-d]_command) will be run.
