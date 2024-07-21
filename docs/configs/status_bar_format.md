related_commands: 

related_configs: 

for_configs: status_bar_format right_status_bar_format status_*_command

demo_code:

doc_body:
The items and format to show in the statusbar. The default value is:
```
[ %{current_page} / %{num_pages} ]%{chapter_name}%{search_results}%{search_progress}%{link_status}%{waiting_for_symbol}%{indexing}%{preview_index}%{synctex}%{drag}%{presentation}%{visual_scroll}%{locked_scroll}%{highlight}%{freehand_drawing}%{rect_select}%{custom_message}%{download}%{download_button}%{network_status}%{tts_status}%{tts_rate}

```
The @config(right_status_bar_format) is similar, but determines what to display on right-aligned statusbar.

Here is what each of the `%{}` variables expand to:
- `%{current_page}` expands to the current page number
- `%{current_page_label}` expands to the the label of the current page (might be different from `current_page` for example some documents have pages labeled with roman numerals, e.g. XII)
- `%{num_pages}` expands to the total number of pages in current document
- `%{chapter_name}` expands to the name of current chapter
- `%{document_name}` expands to the file name of current document
- `%{auto_name}` expands to automatically detected document name
- `%{search_results}` expands to the search results (if there is a search active)
- `%{search_progress}` expands to the current search progress (if there is one in progress)
- `%{link_status}` specifies whether we are linking (that is the source of a @command(portal) has been set and we are waiting for the destination)
- `%{waiting_for_symbol}` specifies if a command is expecting a symbol to be entered
- `%{indexing}` specifies if the current document is currently being indexed
- `%{preview_index}` if there are more than one possible overviews available for the overview window, this option shows how many there are and what is the current index 
- `%{synctex}` displays if we are in synctex mode (@command(toggle_synctex))
- `%{drag}` displays if we are in mouse drag mode (@command(toggle_mouse_drag_mode))
- `%{presentation}` displays if we are in presentation mode (@command(toggle_presentation_mode))
- `%{visual_scroll}` displays if we are in ruler scorll mode (@command(toggle_ruler_scroll_mode))
- `%{locked_scroll}` displays if we have locked horizontal scrolling (@command(toggle_horizontal_scroll_lock))
- `%{highlight}` displays information about the current highlight state (@command(set_select_highlight_type)) 
- `%{freehand_drawing}` displays if we are in freehand drawing mode (@command(toggle_freehand_drawing_mode))
- `%{rect_select}` displays if a command requires a rectangle to be selected
- `%{point_select}` displays if a command requires a point to be selected
- `%{custom_message}` user message that can be set using @command(set_status_string)
- `%{download}` displays if we are currently downloading a paper
- `%{download_button}` a button that displays when the current overview is a reference to a paper, clickin on this button downloads the paper
- `%{closest_bookmark}` displays the closest bookmark to current location
- `%{closest_portal}` displays the closest portal to the current location
- `%{selected_highlight}` displays the comment of the selected highlight
- `%{mode_string}` displays the current mode string (this can be useful when defining @concept(macro)s)
- `%{network_status}` displays the current network status
- `%{tts_status}` displays the current text to speech status
- `%{tts_rate}` displays the current text to speech speed
- `%{custom_message_a}` to `%{custom_message_d}` expands to a custom message which can be set using @config(status_custom_message_[a-d])

Note that clicking on many of these items performs a relevant command. For example clicking on the text expanded from `${current_page}` shows a page-selection widget or clicking on `%{chapter_name}` opens the table of contents. The exact command executed when some part of the statusbar is clicked can be configured using @config(status_*_command) config options where `*` can be any of the options described above. For example if we have the following in the `prefs_user.config` file:

```
status_num_pages_command toggle_dark_mode
```

then clicking on the number of pages in the statusbar toggles dark mode.
