#pragma once

#include <optional>
#include <string>
#include "controllers/base_controller.h"
#include "book.h"

class Path;

class NavigationController : public BaseController{

private:
    // List of previous locations in the current session. Note that we keep the history even across files
    // hence why `DocumentViewState` has a `document_path` member
    std::vector<DocumentViewState> history;
    // the index in the `history` array that we will jump to when `prev_state` is called.
    int current_history_index = -1;
public:
    NavigationController(MainWidget* parent);
    void open_document(const Path& path, std::optional<float> offset_x = {}, std::optional<float> offset_y = {}, std::optional<float> zoom_level = {}, std::string downloaded_checksum="");
    void open_document(const std::wstring& doc_path, bool load_prev_state = true, std::optional<OpenedBookState> prev_state = {}, bool foce_load_dimensions = false);
    void open_document_at_location(const Path& path, int page, std::optional<float> x_loc, std::optional<float> y_loc, std::optional<float> zoom_level, bool should_push_state=true);
    void push_state(bool update = true);
    void next_state();
    void prev_state();
    void update_current_history_index();
    void set_main_document_view_state(DocumentViewState new_view_state);
    void update_renamed_document_in_history(std::wstring old_path, std::wstring new_path);
    void smart_jump_under_pos(WindowPos pos);
    void long_jump_to_destination(DocumentPos pos);
    void handle_link_click(const PdfLink& link);
    std::vector<OpenedBookInfo> get_all_opened_books(bool include_server_books=true, bool force_full_path=false);
    void handle_open_prev_doc();
    bool overview_under_pos(WindowPos pos);
    void move_vertical(float amount);
    bool move_horizontal(float amount, bool force = false);
    void zoom(WindowPos pos, float zoom_factor, bool zoom_in);
    void return_to_last_visual_mark();
    void goto_overview();
    void overview_to_definition();
    void handle_goto_toc();
    void handle_open_all_docs();
};
