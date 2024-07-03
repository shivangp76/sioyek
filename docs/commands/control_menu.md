Used to control ui menus using keyboard or scripts. Takes the desired action as an input string which can be one of the following:

| Action | Description |
| --- | --- |
| `down` | Move down in menu |
| `up` | move up in menu |
| `page_down` | move to the next page in menu |
| `page_up` | move to the previous page in menu |
| `menu_begin` | move to the first item in menu |
| `menu_end` | move to the last item in menu |
| `select` | select the highlighted item |
| `get` | get the value of selected item (mainly used in scripting) |
| `cursor_backward` | move the text cursor one character backward |
| `cursor_forward` | move the text cursor one character forward |
| `select_backward` | select text one character backward |
| `select_forward` | select text one character forward |
| `move_word_backward` | move the text cursor one word backward |
| `move_word_forward` | move the text cursor one word forward |
| `select_word_backward` | select the previous word |
| `select_word_forward` | select the next word |
| `move_to_end` | move to the end of text input |
| `move_to_begin` | move to the start of text input |
| `select_to_end` | select to the end of text input |
| `select_to_begin` | select to the start of text input |
| `select_all` | select all of the text input |
| `delete_to_end` | delete text from cursor location to the end of the input |
| `delete_to_begin` | delete text from the start of the input to the cursor |
| `delete_next_word` | delete the next word after cursor |
| `delete_prev_word` | delete the previous word before cursor |
| `delete_next_char` | delete the character after cursor |
| `delete_prev_char` | delete the character before cursor |
| `next_suggestion` | show the next autocomplete suggestion |
| `prev_suggestion` | show the previous autocomplete suggestion |

For example, suppose you want to use `<C-j>` and `<C-k>` keybinds to move up and down in a menu. You can add the following to your `keys_user.config`:

```
control_menu('down') <C-j>
control_menu('up') <C-k>
```