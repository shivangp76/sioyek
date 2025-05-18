#pragma once

#include <QAbstractScrollArea>
#include <QTextDocument>
#include "types/common_types.h"

class MainWidget;
class MyLineEdit;
class TouchChat;
class QVBoxLayout;
class QPaintEvent;
class QMouseEvent;

class SioyekChatTextBrowser : public QAbstractScrollArea {
    Q_OBJECT
public:
    static const int margin = 10;
    static const int spacing = 10;
    static const int user_inner_margin = 5;
    QFont chat_font;
    SioyekChatTextBrowser(QWidget* parent, QString content);

    //void handle_resize();
    void update_text(QString new_text);
    QString get_selected_text();
signals:
    void anchorClicked(QString anchor_text);

protected:
    void paintEvent(QPaintEvent* event) override;

    void mousePressEvent(QMouseEvent* mevent) override;
    void mouseMoveEvent(QMouseEvent* mevent) override;
    void mouseReleaseEvent(QMouseEvent* mevent) override;
    void mouseDoubleClickEvent(QMouseEvent* mevent) override;
    void resizeEvent(QResizeEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;


private:
    void update_scrollbar(int contentHeight);

    QList<ChatMessage> messages;
    int full_width = 0;
    int response_content_width = 0;
    int user_box_width = 0;
    int user_content_width = 0;

    std::unordered_map<int, std::unique_ptr<QTextDocument>> cached_documents;

    QTextDocument* get_document_for_index(int index);
    QTextDocument* get_doc_and_size_values_for_index(int index, int* out_box_width, int* out_box_height, int* out_box_x, int* out_inner_margin);
    std::pair<int, int> get_cursor_message_and_char_index(QPoint cursor_pos, int forced_message_index=-1);


    int selection_message_index = -1;
    int selection_start_pos = -1;
    int selection_end_pos = -1;
    bool is_selecting = false;

};

class SioyekBookmarkTextBrowser : public QWidget {
    Q_OBJECT
private:
    MainWidget* main_widget = nullptr;
    //SioyekDocumentationTextBrowser* text_browser = nullptr;

    QVBoxLayout* layout = nullptr;
    int last_set_scroll_amount = 0;


protected:
    void resizeEvent(QResizeEvent* resize_event) override;
public:
    bool follow_output = true;
    bool is_bookmark_pending = false;
    bool is_pending = false;
    MyLineEdit* line_edit = nullptr;
    QString bookmark_uuid;
    SioyekChatTextBrowser* text_browser = nullptr;
    TouchChat* touch_text_browser = nullptr;

    SioyekBookmarkTextBrowser(MainWidget* parent, QString bookmark_uuid, QString content, bool chat);

    // void handle_resize();
    Q_INVOKABLE QRect get_prefered_rect(QRect parent_rect);
    void update_text(QString new_text);
    void set_follow_output(bool val);

    void scroll_amount(int amount);
    void scroll_to_start();
    void scroll_to_end();
    void set_pending(bool pending);
    ~SioyekBookmarkTextBrowser();

};
