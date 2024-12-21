# add_annot_to_highlight
Allows the user to select a highlight to add annotations to using the keyboard. A label is displayed next to each visible highlight, and the user can press the corresponding key to select the highlight. Then a text box will appear where the user can type the annotation.

# add_bookmark
Add an invisible bookmark in the current location. You can later search the bookmarks using the [`goto_bookmark`](compiled_commands.md#goto_bookmark-goto_bookmark_g) command.

# add_freetext_bookmark, add_freetext_bookmark_auto
Add a freetext bookmark in the selected rectangle. The [`add_freetext_bookmark`](compiled_commands.md#add_freetext_bookmark-add_freetext_bookmark_auto) prompts the user to select the rectangle while the [`add_freetext_bookmark_auto`](compiled_commands.md#add_freetext_bookmark-add_freetext_bookmark_auto) command doesn't ask for a rectangle, instead it automatically detects possible large empty spaces in the current screen and asks the user to pick one of those. Freetext bookmarks are visible in the document. You can change the display style of the bookmark using the following prefixes:

- If the bookmark text starts with `#` followed by a symbol, it will draw a box with the symbol's color (the same color as the highlighs of the symbol).
- If the bookmark text starts with `#markdown`, the rest of the bookmark will be rendered as markdown
- If the bookmark text starts with `#latex`, the rest of the bookmark will be rendered as latex
- You can change the border/background/text color of the bookmark by adding the corresponding symbol prefix after the `#` or `#markdown` or `#latex` prefix. For example, `#rgb this is bookmark text` will create a bookmark with a red border and green background and blue text. Similarly you can edit the colors of a markdown bookmark like this: `#markdownrgb this is **markdown** bookmark text`.
- If the bookmarks text starts with `? `, it will be interpreted as a question to be asked from the current document (using a large language model). The response will be displayed in the bookmark (this feature is only available to pro users).
- If the bookmark text starts with `#shell` followed by a shell command, then sioyek will run the shell command and fill the bookmark with the output of the shell command. The following variables will be expanded in the shell command:
    * `%{text}` will expand to the text of the bookmark after the `#shell` line (following the newline character). For example, `#shell ping %{text}\ngoogle.com `
    will run `ping google.com` and display the output in the bookmark. Obviously, this is not useful when running the `#shell` bookmark directly but it is useful when defining predefined shell commands using [`new_shell_bookmark_command`](compiled_configs.md#).
    * `%{document_text_file}` expands to the path of a temporary file containing the current document's text.
    * `%{current_page_begin_index}` expands to the index of the first character of the current page in the document text file.
    * `%{current_page_end_index}` expands to the index of the last character of the current page in the document text file.
    * `%{bookmark_image_file}` expands to the path of a temporary file containing the image of the document section where the bookmark is located.
    * `%{selected_text}` expands to the text of the selected text in the document.
- If the bookmark text starts with `@cmd` where `cmd` is a command previously defined using [`new_shell_bookmark_command`](compiled_configs.md#), then sioyek will run the command and fill the bookmark with the output of the command.

# add_highlight
Highlight the selected text. Requires a symbol which specifies the color of the highlighted text. For example executing [`add_highlight`](compiled_commands.md#add_highlight) followed by pressing `a` will create a highlight with color [`highlight_color_a`](compiled_configs.md#). You can configure the highlight color of all highlights in your config files. The highlights can later be searched using [`goto_highlight`](compiled_commands.md#goto_highlight-goto_highlight_g).

# add_highlight_with_current_type
Highlight the selected text with the current highlight type as set by [`set_select_highlight_type`](compiled_commands.md#set_select_highlight_type).

# add_keybind
Dynamically add a new keybinding using the same syntax as a line in `keys.config` file.

# add_marked_bookmark
Add a marked bookmark in the selected location. Marked bookmarks are visible in the document with a small icon. You can later search the bookmarks using the [`goto_bookmark`](compiled_commands.md#goto_bookmark-goto_bookmark_g) command.

# cancel_all_downloads
Cancel all pending downloads.

# change_highlight
Alows the user to change highlight type using keyboard. The user first enters the label of the highlight they want to change, then they enter the label of the new highlight type they want to change it to.

# citers
Show citers of the current paper.

# clear_current_page_drawings, clear_current_document_drawings
Clear all freehand drawings in the current page/document.

# clear_scratchpad
Clear the scratchpad.

# close_overview
Close the overview window.

# close_window
Close the current sioyek window (doesn't close the application if there are more than one window open).

# command
Open a searchable list of all commands. Appending or prepending a `?` to the search query will open the documentation for the selected command. If the query starts with `=` it will show a list of `setconfig` commands which are used to dynamically set configurations, changes made using `setconfig` are not permenant. If you want to permenantly set a config, you can enter `+=` which will search for `setsaveconfig` commands which persistantly set configurations. Alternatively if you want to save the current value of a configuration you can enter `+` which will search the `saveconfig` commands which saves the current value of the selected configuration. If the query starts with `!` it will show a list of `toggleconfig` commands which are used to toggle the value of boolean configurations.

# command_palette
Show a command palette with human-readable command names.

# control_menu
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

# convert_to_latex
Convert the image of the selected rectangle to LaTeX code.

# copy
Copy the selected text to clipboard.

# copy_drawings_from_scratchpad
Copy drawings in the selected rectangle from the scratchpad to the main document.

# copy_link
Displays a symbol next to each visible link and copies the URL of selected link.

# copy_screenshot_to_clipboard
Copy a screenshot of the selected rectangle to the clipboard.

# copy_screenshot_to_scratchpad
Copy a screenshot of the document to the scratchpad (for example can be used to copy an image of an equation to the scratchpad).

# create_fulltext_index_for_current_document
Add the current document to the fulltext search indexed. All indexed documents can later be searched using [`search_all_indexed_documents`](compiled_commands.md#search_all_indexed_documents).

# create_visible_portal
Creates a portal with the selected window point as source. The source will be visible on the document. The destination of the portal can later be set using the [`portal`](compiled_commands.md#portal) command. An overview to the destination of the portal can be opened by right clicking on the portal icon.

# delete_all_pdf_annotations, delete_intersecting_pdf_annotations
Delete all of the built-in PDF annotations (this is different from sioyek annotations, here we mean the PDF annotations created in other PDF viewers or using sioyek's embed_annotations). The [`delete_intersecting_pdf_annotations`](compiled_commands.md#delete_all_pdf_annotations-delete_intersecting_pdf_annotations) command can be used to delete only the annotations that intersect with the selected rectangle.

# delete_current_file_from_server
Delete current file from sioyek servers.

# delete_document_from_fulltext_search_index
Delete the current document from the fulltext search index.

# delete_freehand_drawings
Delete the freehand drawings in the selected rectangle.

# delete_highlight_under_cursor
Delete the highlight under mouse cursor.

# delete_portal, delete_bookmark, delete_highlight
Delete the selected portal/bookmark/highlight. If no item is selected then we delete the closest portal/bookmark/highlight to the current location.

# delete_visible_bookmark
Delete a visible bookmark on the screen using keyboard by specifying the label of the bookmark.

# documentation_search
Search the sioyek documentation.

# donate
Make a donation to support the development of sioyek.

# download_paper_with_url, download_clipboard_url
Download and open a paper from an input URL ([`download_paper_with_url`](compiled_commands.md#download_paper_with_url-download_clipboard_url)) or the URL in clipboard ([`download_clipboard_url`](compiled_commands.md#download_paper_with_url-download_clipboard_url)).

# download_link
Download a paper by selecting its link using the keyboard.

# download_overview_paper, download_overview_paper_no_prompt
Downloads the paper currently shown in overview window. The [`download_overview_paper`](compiled_commands.md#download_overview_paper-download_overview_paper_no_prompt) command first displays the automatically detected paper title and asks for confirmation. The [`download_overview_paper_no_prompt`](compiled_commands.md#download_overview_paper-download_overview_paper_no_prompt) command downloads the paper without asking for confirmation.

# download_paper_under_cursor
Download the paper under mouse cursor. This can be the actual name of the paper (either in the document itself or the overview window) or a reference to the paper (e.g. executing [`download_paper_under_cursor`](compiled_commands.md#download_paper_under_cursor) while cursor is on a reference like `[12]` will download the paper referenced by `[12]`).

# download_paper_with_name
Download a paper given its name as input.

# download_unsynced_files
Download all files on sioyek server that are not available locally.

# edit_portal
Edit the destination of the portal with the closest source. When [`edit_portal`](compiled_commands.md#edit_portal) is executed, sioyek jumps to the destination of the portal, now we can move around the destination and when we are done, we execute [`prev_state`](compiled_commands.md#next_state-prev_state) which updates the destination with the current location and brings us back to the source of the portal.

# edit_selected_bookmark
Edits the selected bookmark (a bookmark can be selected by clicking on it).

# edit_selected_highlight
Edits the annotation of selected highlight (highlights can be selected by clicking on them).

# edit_visible_bookmark
Edit a visible bookmark on the screen using keyboard by specifying the label of the bookmark.

# embed_annotations
Embeds the sioyek annotations into the document as native PDF annotations. This is useful when sharing the file with others who do not have sioyek installed.

# embed_selected_annotation
Embed the selected sioyek annotation into an embedded PDF annotation that can be viewed in other PDF viewers.

# enter_password
Enter the password for password-protected PDF files.

# escape
Simulates pressing escape (e.g. unselects text, exits ruler mode, etc.).

# execute, execute_shell_command
Executes the given shell command. The following substrings will be expanded in the command:

- `%{file_path}` will expand to the full path of current file.
- `%{file_name}` will expand to current file name.
- `%{selected_text}` will expand to the current selected text.
- `%{selection_begin_document}` and `%{selection_end_document}` will expand to the document location of the begin and end of current text selection. The format is `page x y`.
- `%{page_number}` will expand to the current page number.
- `%{command_text}` will ask the user for a text input and expand to the input.
- `%{mouse_pos_window}` will expand to the mouse position in the window. The format is `x y`.
- `%{mouse_pos_document}` will expand to the mouse position in the document. The format is `page x y`.
- `%{paper_name}` will expand to the current file's automatically detected paper/document title
- `%{sioyek_path}` will expand to the path of sioyek executable
- `%{local_database}` will expand to the path the local database file
- `%{shared_database}` will expand to the path the shared database file
- `%{selected_rect}` will expand to the selected rectangle in the document. The format is `page x1 y1 x2 y2`.
- `%{zoom_level}` will expand to the current zoom level.
- `%{offset_x}` and `%{offset_y}` will expand to the current absolute offsets
- `%{offset_x_document}` and `%{offset_y_document}` will expand to the current document offsets
- `%{line_text}`: if we are in ruler mode will expand to the text of current ruler line

# execute_macro
Execute the given sioyek commands string. This is mainly used for scripting and it is probably not directly useful for end-users.

# exit_ruler_mode, close_visual_mark
Exit ruler mode.

# export
Export annotation data into a `JSON` file.

# external_search
Search the selected text using the configured external search engine. This is a symbol command, and each symbol could be configured for a different search engine using @config(search_url_[a-z]). For example in the default configuration `g` searches in google and `s` searches in google scholar. If no text is selected, we search the title of the current document instead (e.g. in the default configuration, to search the current paper in google scholar you can press `ss` when no text is selected).

# extract_table, extract_table_with_prompt
Extract the contents of selected table.
The [`extract_table_with_prompt`](compiled_commands.md#extract_table-extract_table_with_prompt) command asks the user for a natural language prompt and extracts the table according to that (for example you can say something like this: "extract the contents of this table into a latex table").

# find_references
If a piece of text is selected, this command will search for PDF links that point to a location in the span of the selected text. Otherwise this command will allow you to select a PDF link visible in the current screen and finds other PDF links with the same destination. This is useful e.g. to find all references to a specific paper in a document.

# fit_epub_to_window
Fit an epub document exactly to the window dimensions.

# fit_to_overview_width
Fit the overview page such that it fills the width of the screen.

# fit_to_page_width_ratio
Fit to current document to a ratio of screen configured using [`fit_to_page_width_ratio`](compiled_configs.md#fit_to_page_width_ratio). This is useful for users with very wide screens.

# fit_to_page_width, fit_to_page_height, fit_to_page_width_smart, fit_to_page_height_smart, fit_to_page_smart
Fits the document to page width/height. The smart variants try to ignore the white page margins while fitting. [`fit_to_page_smart`](compiled_commands.md#fit_to_page_width-fit_to_page_height-fit_to_page_width_smart-fit_to_page_height_smart-fit_to_page_smart) command fits the document to the minimum of width and height, ignoring the white margins.

# focus_text
Create the ruler under the line with the given text.

# force_download_annotations
Download all unsynced annotations from the server. This is usually performed automatically but can be manually triggered with this command.

# framebuffer_screenshot
Take a screenshot from sioyek framebuffer (ignoring other widgets) and save it to the given path. This is mainly used for testing purposes.

# fulltext_search_current_document
Perform a fulltext search in the current document. The document must be indexed for fulltext search. To index the current document, use [`create_fulltext_index_for_current_document`](compiled_commands.md#create_fulltext_index_for_current_document). To automatically index all documents, set [`automatically_index_documents_for_fulltext_search`](compiled_configs.md#automatically_index_documents_for_fulltext_search) to `1`.

# fuzzy_search
Perform a fuzzy search in the current document. The search string doesn't have to match the text exactly. We try to find the closest matches.

# generic_delete
Allows the user to delete annotations using keyboard. The user enters the label of the annotation they want to delete.

# generic_select
Allows the user to select annotations using keyboard. The user enters the label of the annotation they want to select.

# get_annotations_json
Returns the current document's annotations as a json string.

Example:
```python
from sioyek import Sioyek
import json
s = Sioyek('')
annotations = json.loads(s.get_annotations_json())
print(annotations)
```

# get_config_no_dialog
Returns the current value of given config without displaying anything to the user. This is mainly useful for scripting.

Example:

```python
from sioyek import sioyek
s = sioyek.Sioyek('')
print(s.get_config_no_dialog('highlight_color_a')) # 'color3 0.94 0.64 1\n'
```

# get_config_value
Displays the value of the given config string to the user.

# get_mode_string
Returns the string for current mode. See `mode` for more information.

Example:

```python
from sioyek import Sioyek
s = Sioyek('')
print(s.get_mode_string()) # 'RXHQEDPOSFMT'
```

# get_overview_paper_name
Returns the title of the paper in current overview window. This is useful for scripting and probably not directly useful for end-users.

Example:
```python
from sioyek import Sioyek
s = Sioyek('')
paper_name = s.get_overview_paper_name()
s.set_status_string(f'Current overview paper name is: {paper_name}')
```

# get_paper_name
Returns the title of the current document. This is useful for scripting and probably not directly useful for end-users.

Example:
```python
from sioyek import Sioyek
s = Sioyek('')
paper_name = s.get_paper_name()
s.set_status_string(f'Current document name is: {paper_name}')
```

# get_state_json
Returns a json string containing the current state of the application. The state is a list of json objects for each sioyek window. Each object contains the following fields:

- `document_checksum`: MD5 checksum of the document
- `document_path`: Path to the document
- `loaded_documents`: List of the paths of loaded documents
- `page_number`: Current page number 
- `searching`: Whether the application is currently searching
- `selected_text`: The currently selected text
- `window_height`: Height of the window
- `window_width`: Width of the window
- `window_id`: The id of the window
- `x_offset` and `y_offset`: Current position in document
- `x_offset_in_page` and `y_offset_in_page`: Current position in page
- `zoom_level`: Current zoom level

Example:
```python
from sioyek import sioyek
import json

s = sioyek.Sioyek('')
state = json.loads(s.get_state_json())
print(state['page_number'])
```

# goto_beginning, goto_end
Move to the beginning/end of the document. If the repeat count of the command is `n!=0` then we jump to page `n` (vim style).

# goto_bookmark, goto_bookmark_g
[`goto_bookmark`](compiled_commands.md#goto_bookmark-goto_bookmark_g) opens a searchable list of current document's bookmarks. [`goto_bookmark_g`](compiled_commands.md#goto_bookmark-goto_bookmark_g) opens a searchable list of all documents' bookmarks.

# goto_definition
Go to the the definition of the first symbol in the highlighted line using the `ruler`.

# goto_highlight, goto_highlight_g
Open a searchable list of all highlights in the current document. You can limit the search to highlights of a certain type by prefixing the query by the type of the highlight followed by a space. For example `j {query}` searches for `{query}` only in highlights of type j. [`goto_highlight_g`](compiled_commands.md#goto_highlight-goto_highlight_g) opens a searchable list of all documents' highlights.

# goto_left, goto_right, goto_left_smart, goto_right_smart
Move to the left/right side of the document. The `_smart` variants ignore the white page margins.

# goto_mark
Goto a mark previously set by [`set_mark`](compiled_commands.md#set_mark). See [`set_mark`](compiled_commands.md#set_mark) for more details.

# goto_next_highlight, goto_prev_highlight
Jump to the next/previous highlight in the current document.

# goto_next_highlight_of_type, goto_prev_highlight_of_type
Jump to the next/previous highlight with the current highlight type (as set by [`set_select_highlight_type`](compiled_commands.md#set_select_highlight_type)).

# goto_overview
Jump to the location of current overview.

# goto_page_with_label
Prompts the user for a page label and jumps to that page. The label can be different from the page number in some documents (e.g. some documents have some pages labeled as "i", "ii", "iii", etc.).

# goto_page_with_page_number
Prompts the user for a page number and jumps to that page.

# goto_portal
Go to the destination of the portal with the closest source.

# goto_portal_list
Open a list of portals in the current document.

# goto_selected_text
Jump to the location of selected text.

# goto_tab
Opens a searchable list of tabs.

# goto_toc
Open the table of contents.

# goto_top_of_page, goto_bottom_of_page
Go to the top/bottom of the current page.

# goto_window
Open a searchable list of current sioyek windows.

# import
Import annotation data from an exported json file using [`export`](compiled_commands.md#export).

# import_annotations
Imports the native PDF annotations into sioyek annotations.

# import_local_database, import_shared_database
Import the data from `local.db` or `shared.db` file.

# increase_freetext_font_size, decrease_freetext_font_size
Increase/decrease the font size of the freetext bookmark being edited.

# increase_tts_rate, decrease_tts_rate
Increase/decrease the rate of the text-to-speech (TTS) engine by [`tts_rate_increment`](compiled_configs.md#tts_rate_increment).

# keyboard_overview
Displays a tag next to each word in the document, opens an overview to the selected tag's word (this works even for documents that don't have links, otherwise [`overview_link`](compiled_commands.md#overview_link) would be used).

# keyboard_select
Displays a tag next to each word in the document, selects the text corresponding to the entered begin and end tag (separated by a space). If only a single tag is entered, then we just select that word.

# keyboard_select_line
Select a line using keyboard. Show a label next to each visible line and create a ruler under that line when a label corresponding to that line is entered.

# keyboard_smart_jump
Displays a tag next to each word in the document, jumps to the selected tag's word (this works even for documents that don't have links, otherwise [`overview_link`](compiled_commands.md#overview_link) would be used).

# keys
Opens the default `keys.config` file.

# keys_user
Opens the default `keys_user.config` file.

# keys_user_all
Shows a list of all considered `keys_user.config` files, sorted by priority. All of these files are applied but the files with higher priority override the lower-priority files if they contain the same setting.

# load_annotations_file, import_annotations_file_sync_deleted
Load the annotations from the file created using [`write_annotations_file`](compiled_commands.md#write_annotations_file). The `load_annotations_file` doesn't delete anything from the current file but the `import_annotations_file_sync_deleted` will overwrite the current annotations with the annotations in the file and delete any annotations that are not in the file.

# load_scratchpad
Load a previously saved scratchpad state. This state should have been previously saved using [`save_scratchpad`](compiled_commands.md#save_scratchpad).

# login
Login into your sioyek account.

# login_using_access_token
Login to sioyek servers using previously obtained access token.

# logout
Logout from your sioyek account.

# maximize
Maximize the sioyek window.

# move_left, move_right, move_down, move_up
Basic movement in the current document.

# move_left_in_overview, move_right_in_overview
Move left and right in the overview (you can use the normal up and down movement commands to move up and down).

# move_ruler_up, move_ruler_down, move_visual_mark_up, move_visual_mark_down
Move the ruler up or down.

# move_ruler_to_next_block, move_ruler_to_prev_block
Move the ruler to previous/next text block.

# move_ruler_next, move_ruler_prev, move_visual_mark_next, move_visual_mark_prev
Move the ruler such that more content is visible. This is the same as [`move_ruler_down`](compiled_commands.md#move_ruler_up-move_ruler_down-move_visual_mark_up-move_visual_mark_down) [`move_ruler_up`](compiled_commands.md#move_ruler_up-move_ruler_down-move_visual_mark_up-move_visual_mark_down) commands when the ruler is entirely visible, but when part of the ruler is offscreen (e.g. in mobile devices with a small screen) this command moves such that the obscured part of the ruler becomes visible and only moves to the next line when the entire line has been viewed.
A red mark highlights the leftmost part of the next ruler position, so when you reach the red mark you can run [`move_ruler_next`](compiled_commands.md#move_ruler_next-move_ruler_prev-move_visual_mark_next-move_visual_mark_prev) and continue reading from the left side of the screen.

# new_window
Create a new sioyek window.

# next_overview, previous_overview
Go to the next/prevoius possible overview desitnation. For example, when executing [`overview_definition`](compiled_commands.md#overview_definition) there might be more than one possible target. This command will jump to the next/previous target.

# next_chapter, prev_chapter
Go to the next/previous chapter in table of contents.

# next_item, previous_item
Go to the next/previous search result.

# next_page, previous_page
Move to the next/previous page.

# next_state, prev_state
Move forward/backward in history. Note that this history works even across files (i.e. if you go to a different file you can return to the previous file by executing [`prev_state`](compiled_commands.md#next_state-prev_state) or go back forward to the new file by executing [`next_state`](compiled_commands.md#next_state-prev_state).)

# noop
Do nothing (for example can be used to unbind default keybindings, by binding them to `noop` instead).

# open_containing_folder
Open the folder containing the current file in file explorer.

# open_document
Open the operating system's file explorer to select a document to open.

# open_document_embedded
Opens an embedded file browser in sioyek (unlike [`open_document`](compiled_commands.md#open_document) which uses the operating system's file browser).

# open_document_embedded_from_current_path
Opens an embedded file browser in sioyek rooted at the current opened file's directory with the current file being selected.

# open_external_text_editor
Open an external text editor to edit the current text input. For example you can open an external editor to edit bookmarks or highlight annotations. The external editor can be configured using [`external_text_editor_command`](compiled_configs.md#external_text_editor_command), the corresponding text input will be updated in sioyek whenever the file is saved.

# open_last_document
Opens the last document that was opened in the current sioyek window.

# open_link
Opens PDF links using keyboard. Displays a symbol next to each visible link and jumps when user enters the link symbol.

# open_local_database_containing_folder, open_shared_database_containing_folder
Open the folder containing the local/shared database file.

# open_prev_doc
Shows a searchable list of previously opened documents.

# open_server_only_file
Open a list of files that the user has previously uploaded on sioyek servers but are not available on the current device.

# overview_definition
Open an overview to the definition of the first symbol in the highlighted line using the `ruler`.

# overview_link
Create an overview to a PDF link using keyboard. Displays a symbol next to each visible link and opens an overview to link destination when user enters the link symbol.

# overview_next_item, overview_prev_item
Open an overview to the next/previous search result.

# overview_to_portal
Open an overview to the closest portal.

# overview_to_ruler_portal, goto_ruler_portal
Open an overview/Jump to the portal on current ruler line.

# overview_under_cursor
Open the overview window to the location of reference under mouse cursor. This works even when the document doesn't have links.

# pin_overview_as_portal
Create a "pinned portal" from the current overview. Pinned portals appear as part of the page, you can think of them as an overview that is embedded in a specific part of the page. For example you can pin a figure to the right side of a page describing the figure.

# portal
Creates a new pending portal with the current location as source. If we already have a pending-portal, create the portal with the current location as the destination. As an example use-case you can create a portal from a part of text describing an equation or table to the equation/table itself, the contents of the destination portal can be viewed in a separate window using [`toggle_window_configuration`](compiled_commands.md#toggle_window_configuration) or an overview to see the contents of the portal in the same window using [`overview_to_portal`](compiled_commands.md#overview_to_portal).

If this command is executed while an `overview` is visible, then it creates a "pinned portal" from the current location to the overview. Pinned portals are portals that are displayed in a box like overviews so their destination content is always visible.

# portal_to_definition
Create a portal to the definition of the first symbol in the highlighted line using the `ruler`.

# portal_to_link
Displays a symbol next to each visible link and creates a portal from the current location to the selected link's location.

# portal_to_overview
Create a portal from the current location to the overview location.

# prefs
Opens the default `prefs.config` file.

# prefs_user
Opens the default `prefs_user.config` file.

# prefs_user_all
Shows a list of all considered `prefs_user.config` files, sorted by priority. All of these files are applied but the files with higher priority override the lower-priority files if they contain the same setting.

# quit
Quits sioyek (closes all windows).

# regex_search
Search the contents of current document using regular expressions. You can navigate the results the same way you would with [`search`](compiled_commands.md#search-chapter_search) results.

# reload
Reload all of the current file's data. This can be useful for situations where the file has been modified outside of sioyek, although sioyek will try to automatically reload the file if it detects changes on rare occasions it might fail to do so, in such situations you can use this command to manually reload the file.

# reload_config
Reloads the config files. Sioyek automatically reloads the config files when they are changed but in rare situations when it doesn't do so automatically (e.g. due to a permission issue), you can manually reload them using this command.

# reload_no_flicker
This is similar to the [`reload`](compiled_commands.md#reload) with the difference that here we don't immediately delete the previous renders (which causes the screen to flicker before the new document is rendered).

# rename
Rename the current file. Sioyek tries to fill the default value to an automatically detected title.

# resume_to_server
Resume the current file location to the last location in sioyek servers.

# resync_document
Re-sync current document's annotations with sioyek servers. This command is usually performed automatically but can be manually triggered with this command.

# rotate_clockwise, rotate_counterclockwise
Rotate the document.

# ruler_under_cursor, visual_mark_under_cursor
Create a ruler under the line under mouse cursor.

# save_scratchpad
Save the current scratchpad state to a file. This file can be loaded later using [`load_scratchpad`](compiled_commands.md#load_scratchpad).

# screenshot
Take a screenshot from sioyek window and save it to the given path. This is mainly used for testing purposes.

# screen_down, screen_up
Move one screen up or down. The amount of movement can be configured using [`move_screen_ratio`](compiled_configs.md#move_screen_ratio-move_screen_percentage).

# scroll_selected_bookmark_down, scroll_selected_bookmark_up
Scroll the contents of a freetext bookmark up or down.

# search, chapter_search
Search the contents of current document. You can use [`next_item`](compiled_commands.md#next_item-previous_item) and [`previous_item`](compiled_commands.md#next_item-previous_item) to navigate search results. You can optionally specify a page range using the following syntax: `<begin_page,end_page>query`. The [`chapter_search`](compiled_commands.md#search-chapter_search) command automatically fills the page range with the current chapter's page range.

# search_all_indexed_documents
Perform a fulltext search in all documents indexed by [`create_fulltext_index_for_current_document`](compiled_commands.md#create_fulltext_index_for_current_document) or [`automatically_index_documents_for_fulltext_search`](compiled_configs.md#automatically_index_documents_for_fulltext_search).

# select_current_search_match
Selects the current highlighted search result.

# select_freehand_drawings
Select the freehand drawings in the selected rectangle. The selected drawings can be moved by dragging the selection rectangle.

# select_ruler_text
Select the current line highlighted by ruler, you can use the up and down movement commands to move the selection and [`toggle_line_select_cursor`](compiled_commands.md#toggle_line_select_cursor) to toggle between begin and end of the selection. Exit the selection mode by pressing escape (this is similar to vim's visual line mode).

# semantic_search
Performs a semantic search on the current document. This method produces a relatively lower quality result but costs a lot less than [`semantic_search_extractive`](compiled_commands.md#semantic_search_extractive).

Explanation of how this method works for more technical users: This method simply splits the document into paragraph-sized chunks and embeds each chunk using an embedding model. Then it finds the chunks with most similarity to the query.

# semantic_search_extractive
Performs a semantic search on the current document. This method produces a relatively higher quality result than [`semantic_search`](compiled_commands.md#semantic_search) but is more expensive.

# set_freehand_alpha
Set the alpha transparency of freehand drawings.

# set_freehand_thickness
Set the thickness of freehand drawings. The initial thickness value is 1.

# set_freehand_type
Set the color symbol of freehand drawings. This is a symbol command which takes a single character as input. The color of drawing with symbol `s` will be the same color as highlights with symbol `s`.

# set_mark
Set a mark in the current location so we can return to it later using the [`goto_mark`](compiled_commands.md#goto_mark) command. After executing `set_mark` command, sioyek will wait for you to press a symbol, which can be a lowercase or uppercase letter. Lowercase marks are local to the current file but uppercase marks are global. If you press a symbol that is already used as a mark, the previous mark will be overwritten. You can later return to the mark by executing `goto_mark` command with the same symbol. All marks are persistant, meaning they are saved when sioyek closes is reopened.

# set_select_highlight_type
Set the default highlight type. This is mainly used with [`toggle_select_highlight`](compiled_commands.md#toggle_select_highlight) and [`add_highlight_with_current_type`](compiled_commands.md#add_highlight_with_current_type).

# set_status_string, clear_status_string
Set the message in sioyek status bar. This message will be displayed until it is cleared using [`clear_status_string`](compiled_commands.md#set_status_string-clear_status_string). This is mainly useful for scripts and plugins to display messages to the user.

# show_context_menu
Show a context menu below the mouse cursor. The commands shown in the menu can be configured using the following configs (depending on the object under mouse cursor):

- [`context_menu_items`](compiled_configs.md#context_menu_items-context_menu_items_for_links-context_menu_items_for_selected_text-context_menu_items_for_highlights-context_menu_items_for_bookmarks-context_menu_items_for_overview)
- [`context_menu_items_for_links`](compiled_configs.md#context_menu_items-context_menu_items_for_links-context_menu_items_for_selected_text-context_menu_items_for_highlights-context_menu_items_for_bookmarks-context_menu_items_for_overview)
- [`context_menu_items_for_selected_text`](compiled_configs.md#context_menu_items-context_menu_items_for_links-context_menu_items_for_selected_text-context_menu_items_for_highlights-context_menu_items_for_bookmarks-context_menu_items_for_overview)
- [`context_menu_items_for_highlights`](compiled_configs.md#context_menu_items-context_menu_items_for_links-context_menu_items_for_selected_text-context_menu_items_for_highlights-context_menu_items_for_bookmarks-context_menu_items_for_overview)
- [`context_menu_items_for_bookmarks`](compiled_configs.md#context_menu_items-context_menu_items_for_links-context_menu_items_for_selected_text-context_menu_items_for_highlights-context_menu_items_for_bookmarks-context_menu_items_for_overview)
- [`context_menu_items_for_overview`](compiled_configs.md#context_menu_items-context_menu_items_for_links-context_menu_items_for_selected_text-context_menu_items_for_highlights-context_menu_items_for_bookmarks-context_menu_items_for_overview)

# show_custom_options
Given a `|`-separated list of options, displays the options to the user and returns the result. This is used for scripting and is not directly useful for end-users.

Example:
```
from sioyek import Sioyek
s = sioyek.Sioyek('')
result = s.show_custom_options('one|two|three')
s.set_status_string(f'You chose {result}')
```

# show_text_prompt
Displays a text prompt to the user and returns the entered text. This is used for scripting and is not directly useful for end-users.

Example:
```
from sioyek import Sioyek
s = sioyek.Sioyek('')
result = s.show_text_prompt('Enter a string:')
s.set_status_string(f'You entered {result}')
```

# show_touch_draw_controls
Show the freehand drawing menu for touch devices.

# show_touch_highlight_type_select
Show the highlight type selector for touch devices.

# show_touch_main_menu
Show the main menu for touch devices.

# show_touch_page_select
Show the page selector for touch devices.

# show_touch_settings_menu
Show the settings menu for touch devices.

# show_touch_ui_for_config
Show the touch UI to configure the given configuration.

# smart_jump_under_cursor
Go to the location of reference under mouse cursor. This works even when the document doesn't have links.

# move_down_smooth, move_up_smooth
Basic movement in the current document. Holding these commands results in a smooth continuous movement in the document unlike [`move_down`](compiled_commands.md#move_left-move_right-move_down-move_up) and [`move_up`](compiled_commands.md#move_left-move_right-move_down-move_up).

# start_reading, stop_reading
Start reading from the current line highlighted by `ruler` using the local text to speech engine.

# start_reading_high_quality
Start reading from the current line highlighted by `ruler` using the sioyek servers' high quality neural text to speech engine.

# synchronize
Synchronize a desynchronized file with the server. On rare occasions files might become desynchronized where sioyek can not automatically decide whether to upload annotations to sioyek servers or not. For example suppose you have two copies of the same file on two different devices and with different annotations, and then you upload one of them to sioyek servers. When the other file is opened on the other device, sioyek will not automatically decide to upload the annotations to the server (because the user might not want that, e.g. perhaps those annotations are very sensitive data that you do not want uploaded). In such situations the other file is in desynchronized state and you can use this command to synchronize it with the server. This command will upload the annotations to the server if they are not already there.

# synctex_forward_search
Given a latex file path and line number, performs a forward search to the corresponding location in the PDF document.

# synctex_under_cursor
Perform a synctex search under mouse cursor (this works regardless of whether we are in synctex mode or not). The search command is configured using [`inverse_search_command`](compiled_configs.md#inverse_search_command).

# synctex_under_ruler
Perform a synctex search to the location of the line highlighted by ruler.

# sync_current_file_location
Upload the current file location to sioyek servers (can later be resumed on other devices with [`resume_to_server`](compiled_commands.md#resume_to_server) command). This command is usually performed automatically (only on files that have been uploaded to sioyek servers) but can be manually triggered with this command.

# toggle_text_mark, move_text_mark_forward, move_text_mark_backward, move_text_mark_forward_word, move_text_mark_backward_word, move_text_mark_down, move_text_mark_up
These commands are used to manipulate the text selection using keyboard. `move_{forward/backward}` commands move the cursor one character forward/backward and `move_{forward/backward}_word` commands move the cursor one word forward/backward, and `move_{down/up}` commands move the cursor one line down/up. `toggle_text_mark` toggles the position of cursor between the start and end of text selection.

# toggle_custom_color
Toggle between light and custom color schemes. The background and text color of custom color scheme can be configured using [`custom_text_color`](compiled_configs.md#custom_text_color-custom_background_color) and [`custom_background_color`](compiled_configs.md#custom_text_color-custom_background_color).

All color configs in sioyek can be specialized for custom color mode by adding the `CUSTOM_` prefix. For example, if we want to adjust the `text_highlight_color` for custom color mode only, we could set `CUSTOM_text_highlight_color`.

# toggle_dark_mode
Toggle between light and dark color schemes.

All color configs in sioyek can be specialized for dark mode by adding the `DARK_` prefix. For example, if we want to adjust the `text_highlight_color` for dark mode only, we could set `DARK_text_highlight_color`.

# toggle_drawing_mask
This is a symbol command which given a symbol `s`, toggles the visibility of freehand drawings with type `s`.

# toggle_freehand_drawing_mode
Toggle freehand drawing mode. In this mode you can use the mouse to draw on the document.

# toggle_fullscreen
Toggle fullscreen mode.

# toggle_highlight_links
Highlight the PDF links.

# toggle_horizontal_scroll_lock
Locks horizontal scrolling. This is useful for touchpad or touch devices when you want to always keep the document centered in the screen and accidental horizontal scrolling is undesirable.

# toggle_line_select_cursor
Toggle between begin and end of line text selection by [`select_ruler_text`](compiled_commands.md#select_ruler_text).

# toggle_menu_collapse
Collapse/expand items in table of contents menu.

# toggle_mouse_drag_mode
Toggle mouse drag mode. When enabled, you can drag the mouse to move the document (instead of text selection).

# toggle_pdf_annotations
Toggles the visibility of PDF annotations (these are different from sioyek annotations, the PDF annotations are embedded in the PDF file itself).

# toggle_pen_drawing_mode
Toggle pen drawing mode. In this mode you can use the stylus to draw on the document.

# toggle_presentation_mode, turn_on_presentation_mode
Toggle/turn on presentation mode, in this mode we fit each slide to the entire page and movement commands move to the next/previous slide.

# toggle_rect_hints
Shows the locations of touch mode shortcut rectangles and the commands that they perform.

# toggle_ruler_scroll_mode, toggle_visual_scroll
Toggles ruler scroll mode. In this mode mouse wheel moves the ruler instead of scrolling the document.

# toggle_scratchpad_mode
Opens a scratchpad, you can use this scratchpad to draw or do calculations (this is mainly useful for touch/stylus devices).

# toggle_scrollbar
Toggle the scrollbar.

# toggle_select_highlight
Toggle select highlight mode. In this mode all selected text will automatically be highlighted with the type set using [`set_select_highlight_type`](compiled_commands.md#set_select_highlight_type).

# toggle_statusbar
Toggle the statusbar.

# toggle_synctex, turn_on_synctex
Toggle/turn on synctex mode. In this mode, right clicking on the PDF file performs an inverse search from the corresponding location in the source latex file (using the configured [`inverse_search_command`](compiled_configs.md#inverse_search_command)).

# toggle_titlebar
Toggle the titlebar of the sioyek window.

# toggle_two_page_mode
Toggles between two-page mode and single-page mode.

# toggle_window_configuration
Toggle helper window. The helper window displays the portal destination of the closest portal source to current location.

# turn_on_all_drawings, turn_off_all_drawings
Turn all drawings on/off.

# undo_delete
Undo the last annotation delete operation.

# undo_drawing
Undo the last drawn freehand drawing.

# upload_current_file
Upload the current file into sioyek servers.

# wait
Wait for the given duration (in miliseconds). This is mainly used for testing purposes.

# wait_for_renders_to_finish, wait_for_search_to_finish, wait_for_indexing_to_finish, wait_for_downloads_to_finish
Wait for all pending renders/searches/indexing to finish. This is mainly used for testing and scripting (for example we may want to wait for renders to finish before we compare a screenshot to the saved screenshot).

# write_annotations_file
Write the annotations of current file into an annotations file next to the current file. The annotations file will have the same name as the current file with a `sioyek.annotations` extension. This is useful when you want to share the annotations with others or move them to another computer.

Alternatively you can set the [`annotations_directory`](compiled_configs.md#annotations_directory) config to a directory path and the annotations file will be written there instead of next to the current file.

# zoom_in, zoom_out, zoom_in_cursor, zoom_out_cursor
Zoom in/out in the current document. The `zoom_in_cursor` and `zoom_out_cursor` commands zoom in/out around the cursor position while the `zoom_in` and `zoom_out` commands zoom in/out around the center of the screen.

# zoom_in_overview, zoom_out_overview
Zoom in/out in the overview window.

