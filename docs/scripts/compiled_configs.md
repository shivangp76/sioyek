# adjust_annotation_colors_for_dark_mode
Automatically adjust annotation colors to be more appropriate in dark/custom color modes.

# align_link_dest_to_top
Normally when jumping to links, we align the middle of the screen with the link destination. If this option is set we align the top of the screen instead.

# allow_horizontal_drag_when_document_is_small
Normally when the document is smaller than screen width, we center in the screen and disallow horizontal mouse dragging to keep the document centered. If this option is set, then we allow the document to be dragged even when it is smaller than the screen width.

# allow_main_view_scroll_while_in_overview
Allow the main document to be scrolled when overview is opened (otherwise we use scrolling outside the overview while the overview window is opened to move between possible overview targets).

# annotations_directory
Normally sioyek keeps some annotations files next to the pdf files (e.g. the freehand drawings file). If you don't want this you can specify the [`annotations_directory`](compiled_configs.md#annotations_directory) and then sioyek keeps the annotations files in that directory.

# autocenter_ruler
Always center the document on the ruler when moving the ruler.

# automatically_download_matching_paper_name
If this config is not set, when downloading papers, we search the paper name and show the user a list of matching papers and download it when the user selects one of the options. If this option is set and one of the found paper matches the query title exactly, we no longer show the user a list and automatically download the matching paper.

# automatically_index_documents_for_fulltext_search
Automatically index opened documents for fulltext search.

# automatically_update_checksum_when_document_is_changed
When the document is changed (e.g. a latex document being edited) automatically update the checksum.

# automatically_upload_portal_destination_for_synced_documents
When uploading a document to sioyek servers, also upload the portal destinations for portals in that document.

# auto_login_on_startup
Automatically login to sioyek servers on startup (if previous credentials are available).

# auto_rename_downloaded_papers
If set, sioyek renames the downloaded papers to an automatically determined file name (e.g. instead of a cryptic name like `23434.234.pdf` it will be renamed to something like `some_paper_title.pdf`).

# background_bookmarks_pixel_budget
Maximum number of pixels allocated to cached freetext bookmarks.

# background_color, custom_color_mode_empty_background_color, dark_mode_background_color
The empty background color in light/dark/custom color modes.

# box_highlight_bookmark_transparency
The transparency of box freetext bookmarks. The freetext bookmarks that start with `#` followed by a capital letter will be rendered as transparent boxes, this config controls the transparency of those boxes. The value should be between 0 and 1, where 0 is fully transparent and 1 is fully opaque.

# case_sensitive_search
If set, we match the search queries in a case-sensitive manner.

# check_for_updates_on_startup
Check for software updates when sioyek starts.

# shift_click_command, control_click_command, command_click_command, hold_middle_click_command, tablet_pen_click_command, tablet_pen_double_click_command, right_click_command, middle_click_command, shift_middle_click_command, control_middle_click_command, command_middle_click_command, alt_middle_click_command, shift_right_click_command, control_right_click_command, command_right_click_command, alt_click_command, alt_right_click_command
`macro` commands to run when the corresponding button is clicked (while possibly holding modifier keys). Example:

```
alt_click_command toggle_dark_mode;toggle_scrollbar;set_status_string('hi there')
```

# collapsed_toc
Open the table of contents in collapsed state by default.

# context_menu_items, context_menu_items_for_links, context_menu_items_for_selected_text, context_menu_items_for_highlights, context_menu_items_for_bookmarks, context_menu_items_for_overview
Items in context menu to show when [`show_context_menu`](compiled_commands.md#show_context_menu) is executed and the cursor is on the object specific to the option ([`context_menu_items`](compiled_configs.md#context_menu_items-context_menu_items_for_links-context_menu_items_for_selected_text-context_menu_items_for_highlights-context_menu_items_for_bookmarks-context_menu_items_for_overview) is for the case where no special object is under the cursor).

If [`right_click_context_menu`](compiled_configs.md#right_click_context_menu) is set, you can open this all these menus (except for [`context_menu_items`](compiled_configs.md#context_menu_items-context_menu_items_for_links-context_menu_items_for_selected_text-context_menu_items_for_highlights-context_menu_items_for_bookmarks-context_menu_items_for_overview)) by right-clicking on objects in the page.

# create_table_of_contents_if_not_exists
If the document doesn't have a table of contents, try to automatically generate one (this is a heuristic and doesn't work perfectly all the time).

# custom_color_contrast
Specifies how much should we try to stick with original colors vs how much we want to improve the contrast for readability. If the value is 0, then we try to stick with the original colors as much as possible. If the value is 1, then we try to improve the contrast as much as possible. And the intermediate values are a mix of these two extremes.

# custom_text_color, custom_background_color
Text and background colors in custom color mode.

# dark_mode_contrast
The color contrast in dark mode.

# default_open_file_path
The default root path when opening files.

# documentation_font_size
Font size of documentation, when viewing the embedded documentation widget.

# document_location_mismatch_strategy
Specifies what to do when there is a mistmatch between a local document and the last location uploaded to sioyek servers.
Possible values are:

- `local`: remain in the local position
- `server`: move to the server position
- `ask`: prompt the user to choose a position
- `show_button`: show a resume button in sioyek statusbar

# dont_center_if_synctex_rect_is_visible
When performing a synctex search, if the target is already visible on the screen we don't move the document to center on the target.

# epub_css
CSS to apply to epub documents.

# epub_width, epub_height, epub_font_size
Configure the appearance of epub documents.

# exact_highlight_select
When adding the selected highlight, add the exact highlighted text (otherwise we highlight an entire word event when a part of it is selected).

# external_text_editor_command
The command line program to use when running [`open_external_text_editor`](compiled_commands.md#open_external_text_editor) command. In this config, `%{file}` expands to the file that should be opened. For example:

```
external_text_editor_command nvim "%{file}"
```

# extract_table_prompt
The plain english prompt to use when extracting table data using a large language model with [`extract_table`](compiled_commands.md#extract_table-extract_table_with_prompt) command.

Example:
```
extract_table_prompt Extract the table in this image into a markdown table. Take your time and make sure the result is 100% correct. Split muticolumns into separate columns.
```

# fill_textbar_with_selected_text
When opening the textbar (e.g. when executing [`search`](compiled_commands.md#search-chapter_search) or any other command that requires text input) automatically fill the input with the selected text.

# fit_to_page_width_ratio
The screen width ratio when using [`fit_to_page_width_ratio`](compiled_commands.md#fit_to_page_width_ratio) command.

# flat_toc
Show the table of contents as a flat list instead of a hierarchial tree. This can improve the performance on documents with extremely large table of contents.

# font_size
The font size for UI menus.

# freetext_bookmark_color
Text color to use when adding new freetext bookmarks.

# freetext_bookmark_font_size
Font size to use when adding new freetext bookmarks.

# fuzzy_searching
(Deprecated) Whether we should employ fuzzy searching in menus. As we are using a fuzzy algorithm by default in more and more menus this option is becoming less relevant as time goes on.

# gamma
The gamma value, each pixel value `p` will be replace with `p^gamma`. Higher gamma values make fonts look thicker and lower gamma values make them look thinner.

# hide_overlapping_link_labels
When highlighting links in a document with overlapping links, show the label only for one of the overlapping links (avoids clutter in some malformed documents).

# highlight_color_[a-z]
Specifies the color for each of highlight types. For example `highlight_color_r` specifies the color of highlights with symbol `r`.

# highlight_links
Whether we should highlight PDF links.

# highlight_middle_click
If set, middle clicking on selected text highlights it with the current highlight type.

# hover_overview
Open the overview page when mouse cursor hovers over a link (I don't recommend using this config because it might not work well with other sioyek features, just right click on the link to create the overview).

# ignore_scroll_events
Ignore scroll events (e.g. useful to avoid moving in presentation mode when accidentally touching the touchpad).

# ignore_whitespace_in_presentation_mode
Ignore whitespace when fitting pages to screen in presentation mode.

# incremental_search
If this is set, then we perform searches as user is typing the search query.

# initial_snapped_dragging
If set, when dragging the document (e.g. in touch mode) we ignore very small horizontal scrolls because when the user is trying to scroll vertically they probably will also do some horizontal scrollling inadvertently. So we start the dragging in a "snapped" state (meaning we are snapped to the original horizontal position) and only unspan when the user moves horizontally past a threshold.

# inverse_search_command
The command line program to open the corresponding file when peforming a synctex inverse search. `%1` will expand to the path of TeX file and `%2` will exapnd to the line number.

For example here is how to configure it for vscode on windows:
```
inverse_search_command      "C:\path\to\Code.exe" -r -g "%1":%2
```

# inverted_horizontal_scrolling
Invert the horizontal scrolling when using touchpad.

# invert_selected_text
Invert the selected text to highlight it.

# keyboard_point_selection
If set, when executing commands that require a point on the screen to be selected, we show a list of labels on the screen and allow the user to select the point by typing the label using keyboard.

# keyboard_select_background_color, keyboard_select_text_color, keyboard_selected_tag_background_color, keyboard_selected_tag_text_color
The color of the tags displayed for keyboard commands.

# keyboard_select_font_size
The font size of the tags displayed for keyboard commands.

# linear_filter
Perform linear texture filtering instead of nearest neighbor (should not be necessary unless you use non-interger render scales).

# link_highlight_color
Highlight color to use when highlighting links.

# max_created_toc_size
The maximum size of automatically generated table of contents.

# menu_matched_search_highlight
The CSS style used to highlight matched search results in menus.

# menu_screen_width_ratio, menu_screen_height_ratio
The ratio of screen width/height to use for menus.

# middle_click_search_engine, shift_middle_click_search_engine
When middle clicking on a paper name, we will search for it in the engine (as configured in @config(search_url_[a-z])) set here.

For example:
```
search_url_s https://scholar.google.com/scholar?q=
middle_click_search_engine s
```
means that we will search the paper name in google scholar when user middle clicks on a paper name.

# move_screen_ratio, move_screen_percentage
The ratio of screen height to move when executing [`screen_down`](compiled_commands.md#screen_down-screen_up) and [`screen_up`](compiled_commands.md#screen_down-screen_up).

# numeric_tags
When performing `keyboard_` commands that require a point to be selected, we associate a tag with each possible target. If this option is set, the tags only use numerical values instead of alphabetical values.

# num_cached_pages
Total number of cached rendered pages. Higher values may reduce flickering and power consumption (because pages have to be re-rendered less) at the cost of increased memory usage.

# num_prerendered_next_slides, num_prerendered_prev_slides
Number of next/previous pages to prerender in presentation mode.

# num_two_page_columns
Changes the number of page columns in "two" page mode.

# overview_reference_highlight_color
Highlight color to use when highlighting the automatically detected overview paper name.

# page_separator_width, page_separator_color
Color and size of separator between pages.

# page_space_x, page_space_y
Spacing between pages in two page mode.

# papers_folder_path
Path of the directory where downloaded papers should move to.

# paper_download_should_create_portal
When downloading papers, create a portal from the source document to the downloaded document. Otherwise (if this config is 0) we open the downloaded document in a new window.

# prerendered_page_count
Number of pages to pre-render.

# prerender_next_page_presentation
Prerender the next page in presentation mode (avoids flickering when moving to the next slide).

# preserve_image_colors_in_dark_mode, inverted_preserved_image_colors
Preserve original image colors in dark mode and custom color mode.
If [`inverted_preserved_image_colors`](compiled_configs.md#preserve_image_colors_in_dark_mode-inverted_preserved_image_colors) is set, then we invert the original image colors in custom color mode.

# question_bookmark_background_color, question_bookmark_text_color
The background and text color of question bookmarks (the bookmarks starting with `? `, see [`add_freetext_bookmark`](compiled_commands.md#add_freetext_bookmark-add_freetext_bookmark_auto) for more information).

# real_page_separation
The page separator configured using [`page_separator_width`](compiled_configs.md#page_separator_width-page_separator_color) doesn't actually separate the pages, it is simply a box drawn to distinguish the pages. If [`real_page_separation`](compiled_configs.md#real_page_separation) is set, then we actually move the pages to separate them (technically it is less efficient than the other method which is why it is not the default but it shouldn't matter in most cases).
Also note that when this option is set, the distance between pages is not configured with [`page_separator_width`](compiled_configs.md#page_separator_width-page_separator_color) rather it uses [`page_space_y`](compiled_configs.md#page_space_x-page_space_y).

# reload_interval_miliseconds
We check the document for changes (e.g. a latex document might be changed from external sources) every `reload_interval_miliseconds`.

# render_freetext_borders
Render the borders around freetext bookmarks.

# render_pdf_annotations
If set to 0, we don't render PDF annotations (by PDF annotations I mean the native PDF annotations embedded in the PDF file and not the sioyek annotations).

# resize_command
`macro` commands to run when sioyek window is resized.
Example:

```
resize_command fit_to_page_width_smart
```

# right_click_context_menu
Show a context menu when user right clicks on annotations/overview. The commands displayed in this context menu can be configured using [`context_menu_items`](compiled_configs.md#context_menu_items-context_menu_items_for_links-context_menu_items_for_selected_text-context_menu_items_for_highlights-context_menu_items_for_bookmarks-context_menu_items_for_overview) commands.

# ruler_auto_move_sensitivity
In touch mode, you can move the ruler by pressing on the ruler next rect (by default it is in the bottom-right of the screen) and dragging it. This configuration controls how sensitive the ruler is to your touch. A higher value will make the ruler move faster when you drag it. A lower value will make the ruler move slower when you drag it.

# ruler_auto_move_threshold_distance
In touch mode, you are able to move the ruler by pressing on the move down rect and then moving the cursor, each time the cursor moves `ruler_auto_move_threshold_distance` we move to the next line.

# ruler_color
Ruler color.

# ruler_display_mode
Specifies how the ruler should be rendered. The possible values are:

- `box`: draw a box around selected line
- `slit`: draw a shadow on everything except the selected line
- `underline`: draw a line under the selected line
- `highlight_below`: highlight the entire screen area below the line
- `highlight`: highlights the ruler line

# ruler_marker_color
The ruler marker color to highlight the next left side of the screen when using [`move_ruler_next`](compiled_commands.md#move_ruler_next-move_ruler_prev-move_visual_mark_next-move_visual_mark_prev).

# ruler_next_page_end_pos, ruler_next_page_begin_pos
[`ruler_next_page_end_pos`](compiled_configs.md#ruler_next_page_end_pos-ruler_next_page_begin_pos) determines when we should move to the next page when using the ruler and when we do so, [`ruler_next_page_begin_pos`](compiled_configs.md#ruler_next_page_end_pos-ruler_next_page_begin_pos) determines where the new location of the ruler should be in the screen.

[`ruler_next_page_begin_pos`](compiled_configs.md#ruler_next_page_end_pos-ruler_next_page_begin_pos) is relative to the center of the screen and [`ruler_next_page_end_pos`](compiled_configs.md#ruler_next_page_end_pos-ruler_next_page_begin_pos) is relative to the bottom of the screen. For example, [`ruler_next_page_begin_pos`](compiled_configs.md#ruler_next_page_end_pos-ruler_next_page_begin_pos)=1 means we move one unit (each unit is half screen height) up from the middle of the screen which means we move to the top of the screen when moving to the next page.

# ruler_padding, ruler_x_padding
Ruler padding when using `slit` ruler display mode.

# ruler_pixel_width
Ruler thickness when [`ruler_display_mode`](compiled_configs.md#ruler_display_mode) is set to `underline`.

# ruler_slit_color
Ruler color when [`ruler_display_mode`](compiled_configs.md#ruler_display_mode) is set to `slit`.

# save_externally_edited_text_on_focus
If this option is set, when editing text using [`open_external_text_editor`](compiled_commands.md#open_external_text_editor) command, we accept the externally edited text when the sioyek window gains focus again (otherwise the user has to manually press enter to accept the input).

# scrollview_sensitivity
The relative sensitivity when scrolling the overview window.

# scroll_zoom_inc_factor
The amount to multiply/divide the zoom level when executing zoom commands by holding control and using the mouse wheel.

# search_highlight_color
Highligh color of search matches.

# search_url_[a-z]
Search url for [`external_search`](compiled_commands.md#external_search) command. After executing external search, the user can enter the symbol corresponding to the configured search engine to perform the search using that engine.

For example if we have the following in `prefs_user.config`:

```
search_url_g https://www.google.com/search?q=
search_url_s https://scholar.google.com/scholar?q=
```

Then executing [`external_search`](compiled_commands.md#external_search) followed by `s` will search google and if it is followed by `s` it will search google scholar.

# server_and_local_document_mismatch_threshold
When the location of server and document do not match (for example when the user has moved the document to another location in another device) if the distance is greater than this threshold a resume button will be displayed in the statusbar which allows the user to resume to the server location.

# shared_database_path
Path of shared database to use, if not set we use the default path.

# should_highlight_unselected_search
Whether we should highlight the found search results that are not the current search result.

# should_launch_new_window
When opening a new file from command line, use a new sioyek window if one already exists.

# should_load_tutorial_when_no_other_file
When there is no other file to open (e.g. due to sioyek being opened for the first time) show the sioyek tutorial.

# should_warn_about_user_key_override
Print a warning message in console when a key override is detected in `keys_user.config`.

# show_most_recent_commands_first
When opening the command menu show the most recent commands executed in the current session first.

# show_reference_overview_highlights
Highlight the reference paper name in overview window (this is the automatically detected paper name which will be downloaded with [`download_overview_paper`](compiled_commands.md#download_overview_paper-download_overview_paper_no_prompt) command).

# single_click_selects_words
Normally when selecting text using mouse, we select individual characters, unless the user has double clicked to initiate the selection, in which case we select entire words instead of individual characters.

If [`single_click_selects_words`](compiled_configs.md#single_click_selects_words) is set, then we select words even when user uses single-click to initiate text selection.

# sliced_rendering
Normally we render entire pages as a whole. This can cause problems on high zoom levels on low-memory devices becasue a huge chunk of memory with the correct size might not be available. If [`sliced_rendering`](compiled_configs.md#sliced_rendering) is set, instead of rendering pages as a whole, we slice the pages into multiple strips and render each strip.

# smartcase_search
If set, we match the search queries in a case-insensitive manner if all the characters in the query is lowercase and otherwise we perform a case-sensitive search.

# smooth_move_max_velocity
The speed of `move_*_smooth` commands.

# smooth_scroll_mode
When enabled, using the mouse wheel will scroll the document smoothly instead of discrete jumps. The speed of the scroll can be adjusted with the @config(`smooth_scroll_speed`) and the deceleration rate can be adjusted with the @config(`smooth_scroll_drag`).

# smooth_scroll_speed, smooth_scroll_drag
The speed and deceleration rate of the smooth scroll when [`smooth_scroll_mode`](compiled_configs.md#smooth_scroll_mode) is enabled.

# sort_bookmarks_by_location
Sort bookmarks by location (instead of creation date).

# startup_commands
`macro` commands to run then sioyek starts up. For example:

```
startup_commands toggle_dark_mode;toggle_scrollbar;set_status_string(hi there)
```

Will set the color scheme to dark mode and enable scrollbar and show a message in sioyek statusbar when sioyek starts up.

# status_bar_color, status_bar_text_color
Text and background colors of statusbar.

# status_bar_font_size
Font size of the status bar.

# status_bar_format, right_status_bar_format, status_*_command
The items and format to show in the statusbar. The default value is:

```
[ %{current_page} / %{num_pages} ]%{chapter_name}%{search_results}%{search_progress}
%{link_status}%{waiting_for_symbol}%{indexing}%{preview_index}%{synctex}%{drag}
%{presentation}%{visual_scroll}%{locked_scroll}%{highlight}%{freehand_drawing}
%{rect_select}%{custom_message}%{download}%{download_button}%{network_status}
%{tts_status}%{tts_rate}
```


The [`right_status_bar_format`](compiled_configs.md#status_bar_format-right_status_bar_format-status_*_command) is similar, but determines what to display on right-aligned statusbar.

Here is what each of the `%{}` variables expand to:

- `%{current_page}` expands to the current page number
- `%{current_page_label}` expands to the the label of the current page (might be different from `current_page` for example some documents have pages labeled with roman numerals, e.g. XII)
- `%{num_pages}` expands to the total number of pages in current document
- `%{chapter_name}` expands to the name of current chapter
- `%{document_name}` expands to the file name of current document
- `%{auto_name}` expands to automatically detected document name
- `%{search_results}` expands to the search results (if there is a search active)
- `%{search_progress}` expands to the current search progress (if there is one in progress)
- `%{link_status}` specifies whether we are linking (that is the source of a [`portal`](compiled_commands.md#portal) has been set and we are waiting for the destination)
- `%{waiting_for_symbol}` specifies if a command is expecting a symbol to be entered
- `%{indexing}` specifies if the current document is currently being indexed
- `%{preview_index}` if there are more than one possible overviews available for the overview window, this option shows how many there are and what is the current index 
- `%{synctex}` displays if we are in synctex mode ([`toggle_synctex`](compiled_commands.md#toggle_synctex-turn_on_synctex))
- `%{drag}` displays if we are in mouse drag mode ([`toggle_mouse_drag_mode`](compiled_commands.md#toggle_mouse_drag_mode))
- `%{presentation}` displays if we are in presentation mode ([`toggle_presentation_mode`](compiled_commands.md#toggle_presentation_mode-turn_on_presentation_mode))
- `%{visual_scroll}` displays if we are in ruler scorll mode ([`toggle_ruler_scroll_mode`](compiled_commands.md#toggle_ruler_scroll_mode-toggle_visual_scroll))
- `%{locked_scroll}` displays if we have locked horizontal scrolling ([`toggle_horizontal_scroll_lock`](compiled_commands.md#toggle_horizontal_scroll_lock))
- `%{highlight}` displays information about the current highlight state ([`set_select_highlight_type`](compiled_commands.md#set_select_highlight_type)) 
- `%{freehand_drawing}` displays if we are in freehand drawing mode ([`toggle_freehand_drawing_mode`](compiled_commands.md#toggle_freehand_drawing_mode))
- `%{rect_select}` displays if a command requires a rectangle to be selected
- `%{point_select}` displays if a command requires a point to be selected
- `%{custom_message}` user message that can be set using [`set_status_string`](compiled_commands.md#set_status_string-clear_status_string)
- `%{download}` displays if we are currently downloading a paper
- `%{download_button}` a button that displays when the current overview is a reference to a paper, clickin on this button downloads the paper
- `%{closest_bookmark}` displays the closest bookmark to current location
- `%{closest_portal}` displays the closest portal to the current location
- `%{selected_highlight}` displays the comment of the selected highlight
- `%{mode_string}` displays the current mode string (this can be useful when defining `macro`s)
- `%{network_status}` displays the current network status
- `%{tts_status}` displays the current text to speech status
- `%{tts_rate}` displays the current text to speech speed
- `%{custom_message_a}` to `%{custom_message_d}` expands to a custom message which can be set using @config(status_custom_message_[a-d])

Note that clicking on many of these items performs a relevant command. For example clicking on the text expanded from `${current_page}` shows a page-selection widget or clicking on `%{chapter_name}` opens the table of contents. The exact command executed when some part of the statusbar is clicked can be configured using @config(status_*_command) config options where `*` can be any of the options described above. For example if we have the following in the `prefs_user.config` file:

```
status_num_pages_command toggle_dark_mode
```

then clicking on the number of pages in the statusbar toggles dark mode.

# status_custom_message_[a-d], status_custom_message_[a-d]_command
Show custom messages which can be shown in statusbar if `%{custom_message_a}` to `%{custom_message_d}` is added to [`status_bar_format`](compiled_configs.md#status_bar_format-right_status_bar_format-status_*_command). When the message is clicked, the command configured in @config(status_custom_message_[a-d]_command) will be run.

# status_font
Font face to use for menus.

# strike_line_width
The strike width when adding strike highlights (highlights with type `_`).

# super_fast_search
If set, we index the document so that searches can be performed instantaneously. Some other sioyek features work only if this option is enabled so I recommend to keep it enabled (the extra memory requirement due to this option being enabled is extremely small).

# synctex_highlight_color
Highlight color to use when highlighting synctex search results. This only applies when [`use_ruler_to_highlight_synctex_line`](compiled_configs.md#use_ruler_to_highlight_synctex_line) is set to 0, otherwise we use the ruler to highlight the line.

# synctex_highlight_timeout
Duration (in seconds) to highlight the synctex line. If the duration is -1, the highlight will stay indefinitely.

# table_extract_behaviour
What to do with extracted table. The possible values are:

- `bookmark`: Create a freetext bookmark with the extracted table
- `copy`: Copy the extracted table into clipboard

# tag_font_face
Font face to use for keyboard tags.

# text_highlight_color
The highlight color of selected text (e.g. when user selects text using mouse or using commands like [`keyboard_select`](compiled_commands.md#keyboard_select)).

# toc_jump_align_top
When jumping to a table of contents entry, align the top of the screen with the target location (instead of center of the screen).

# touchpad_sensitivity
Relative scrolling sensitivity when using touchpad or mouse wheel, this is a multiplier that modifies [`vertical_move_amount`](compiled_configs.md#vertical_move_amount-horizontal_move_amount) so the overal move amount when using touchpad or mouse wheel is `touchpad_sensitivity * vertical_move_amount`.

# touch_mode
This config is suitable for touch devices, if set, many behaviours of sioyek change to better support touch devices. For exmaple dragging on the screen moves the document instead of selecting text and menus are now optimized for touch devices.

# back_rect_tap_command, back_rect_hold_command, forward_rect_tap_command, forward_rect_hold_command, middle_left_rect_tap_command, middle_left_rect_hold_command, middle_right_rect_tap_command, middle_right_rect_hold_command, ruler_next_tap_command, ruler_next_hold_command, ruler_prev_tap_command, ruler_prev_hold_command
`macro` commands to run when the corresponding rectangle is tapped or held (only works in touch mode). You can view the location of these rectangles by executing executing [`toggle_rect_hints`](compiled_commands.md#toggle_rect_hints).

# tts_rate
The text to speech speaking speed when using text to speech commands.

# tts_rate_increment
How much to increase/decrease the rate of the text-to-speech (TTS) engine by when using the [`increase_tts_rate`](compiled_commands.md#increase_tts_rate-decrease_tts_rate) and [`decrease_tts_rate`](compiled_commands.md#increase_tts_rate-decrease_tts_rate) commands.

# ui_font
Font face to use for menus.

# portrait_back_ui_rect, portrait_forward_ui_rect, landscape_back_ui_rect, landscape_forward_ui_rect, portrait_visual_mark_next, portrait_visual_mark_prev, landscape_visual_mark_next, landscape_visual_mark_prev, landscape_middle_right_rect, portrait_middle_right_rect, landscape_middle_left_rect, portrait_middle_left_rect
Configures the rectangles in touch mode. If the user taps on these rectangles, we execute the corresponding command as configured in [`touch_rect_commands`](compiled_configs.md#back_rect_tap_command-back_rect_hold_command-forward_rect_tap_command-forward_rect_hold_command-middle_left_rect_tap_command-middle_left_rect_hold_command-middle_right_rect_tap_command-middle_right_rect_hold_command-ruler_next_tap_command-ruler_next_hold_command-ruler_prev_tap_command-ruler_prev_hold_command).

# ui_text_color, ui_background_color, ui_selected_text_color, ui_selected_background_color
Text and background colors of ui menus.

# unselected_search_highlight_color
The highlight color to highlight unselected search matches when [`should_highlight_unselected_search`](compiled_configs.md#should_highlight_unselected_search) is set.

# use_ruler_to_highlight_synctex_line
Use the ruler to highlight the synctex search result (instead of highlighting it with a color).

# use_system_theme, use_custom_color_as_dark_system_theme
Try to use system theme for light/dark modes (might not work in some operating systems).
If [`use_custom_color_as_dark_system_theme`](compiled_configs.md#use_system_theme-use_custom_color_as_dark_system_theme) is set, then we use the custom color mode instead of dark mode when system theme is set to dark.

# vertical_move_amount, horizontal_move_amount
The amount to move when executing movement commands (including mouse wheel).

# wheel_zoom_in_cursor
When performing zoom using control+mouse wheel, zoom on the mouse cursor instead of the center of the screen.

# main_window_size, helper_window_size, main_window_move, helper_window_move, single_main_window_size, single_main_window_move, overview_size, overview_offset
Default size and offset of sioyek windows. the `single_*` is used when only the main sioyek window is visible (no helper window) otherwise we use the other values.

# zoom_inc_factor
The amount to multiply/divide the zoom level when executing zoom commands. For example, if the current zoom level is 2 and `zoom_inc_factor` is 1.5, then after executing [`zoom_in`](compiled_commands.md#zoom_in-zoom_out-zoom_in_cursor-zoom_out_cursor), the zoom level will be `2 * 1.5 = 3`.

