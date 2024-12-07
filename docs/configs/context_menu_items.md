related_commands: show_context_menu

related_configs: right_click_context_menu
type: string

demo_code:

for_configs: context_menu_items context_menu_items_for_links context_menu_items_for_selected_text context_menu_items_for_highlights context_menu_items_for_bookmarks context_menu_items_for_overview

doc_body:
Items in context menu to show when @command(show_context_menu) is executed and the cursor is on the object specific to the option (@config(context_menu_items) is for the case where no special object is under the cursor).

If @config(right_click_context_menu) is set, you can open this all these menus (except for @config(context_menu_items)) by right-clicking on objects in the page.
