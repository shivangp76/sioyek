#include <QPainterPath>
#include <QScrollBar>
#include <QAbstractTextDocumentLayout>
#include <QPainter>

#include "touchui/TouchChat.h"
#include "ui/bookmark_ui.h"
#include "main_widget.h"
#include "ui.h"
#include "utils/window_utils.h"
#include "utils/color_utils.h"

extern bool TOUCH_MODE;
extern float CHAT_WINDOW_BACKGROUND_COLOR[3];
extern float CHAT_WINDOW_USER_MESSAGE_BACKGROUND_COLOR[3];
extern float CHAT_WINDOW_TEXT_COLOR[3];
extern float DEFAULT_TEXT_HIGHLIGHT_COLOR[3];
extern float CHAT_WINDOW_USER_TEXT_COLOR[3];
extern float MENU_SCREEN_WDITH_RATIO;
extern float MENU_SCREEN_HEIGHT_RATIO;

QRect get_bookmark_chat_prefered_rect(QRect parent_rect){
    int parent_width = parent_rect.width();
    int parent_height = parent_rect.height();
    return QRect(
        parent_width * (1 - MENU_SCREEN_WDITH_RATIO) / 2,
        parent_height * (1 - MENU_SCREEN_HEIGHT_RATIO) / 2,
        parent_width * MENU_SCREEN_WDITH_RATIO,
        parent_height * MENU_SCREEN_HEIGHT_RATIO
        );
}


SioyekBookmarkTextBrowser::SioyekBookmarkTextBrowser(MainWidget* parent, QString uuid, QString content, bool chat) : QWidget(parent){

    bookmark_uuid = uuid;
    main_widget = parent;
    layout = new QVBoxLayout(this);

    auto handle_anchor_clicked = [this](QString url_string){
        // percent decode the url string
        url_string = QUrl::fromPercentEncoding(url_string.toUtf8());
        if (url_string.startsWith("sioyeklink#")) {
            url_string = url_string.mid(11).trimmed();
            main_widget->pop_current_widget();
            main_widget->push_state();
            main_widget->dv()->perform_fuzzy_search(url_string.toStdWString());
            main_widget->goto_search_result(0);
            main_widget->invalidate_render();
        }
        else if (url_string.startsWith("http")) {
            open_web_url(url_string.toStdWString());
        }
    };

    if (!TOUCH_MODE){
        text_browser = new SioyekChatTextBrowser(this, content);
        QObject::connect(text_browser, &SioyekChatTextBrowser::anchorClicked, handle_anchor_clicked);



        if (chat) {
            line_edit = new MyLineEdit(main_widget);
            line_edit->setPlaceholderText("Chat with the document.");
            line_edit->setParent(this);
            line_edit->setFont(get_chat_font_face_name());

            QColor background_color = convert_float3_to_qcolor(CHAT_WINDOW_USER_MESSAGE_BACKGROUND_COLOR);
            QColor text_color = convert_float3_to_qcolor(CHAT_WINDOW_USER_TEXT_COLOR);

            line_edit->setStyleSheet("QLineEdit{background-color: " + background_color.name() + "; color: " + text_color.name() + "; border-radius: 0px; padding: 10px;}");
            text_browser->setFocusPolicy(Qt::NoFocus);
        }


        layout->addWidget(text_browser);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->setSpacing(0);
        if (chat) {
            layout->addWidget(line_edit);
        }

        layout->setStretch(0, 1);

        if (chat) {
            line_edit->setFocus();
        }
    }
    else{
        touch_text_browser = new TouchChat(this);
        layout->addWidget(touch_text_browser);
        update_text(content);
        QObject::connect(touch_text_browser, &TouchChat::anchorClicked, handle_anchor_clicked);
        QObject::connect(touch_text_browser, &TouchChat::onMessageSend, [this](QString message){
            main_widget->accept_new_bookmark_message_with_text(message);
        });
        setFocusProxy(touch_text_browser);

    }


    setLayout(layout);
}



void SioyekBookmarkTextBrowser::resizeEvent(QResizeEvent* resize_event) {

    QWidget::resizeEvent(resize_event);

    // handle_resize();

    QPainterPath path;
    path.addRoundedRect(rect(), 4, 4);
    setMask(QRegion(path.toFillPolygon().toPolygon()));


}

QList<ChatMessage> get_chat_messages_from_bookmark_text(QString new_text){
    QList<ChatMessage> messages;

    QStringList lines = new_text.split("\n", Qt::KeepEmptyParts);

    QString current_response;
    QString current_question;

    for (auto line : lines) {
        bool is_question = line.startsWith("? ");
        QString actual_line_text = line;
        if (is_question) {
            actual_line_text = line.mid(2);
        }
        if (is_question && current_response.size() > 0) {
            messages.push_back({ ChatMessageType::ResponseMessage, current_response });
            current_response = "";
        }
        if (!is_question && current_question.size() > 0) {
            messages.push_back({ ChatMessageType::UserMessage, current_question });
            current_question = "";
        }

        if (is_question) {
            current_question += actual_line_text + "\n\n";
        }
        else {
            current_response += actual_line_text + "\n";
        }
    }
    if (current_question.size() > 0) {
        messages.push_back({ ChatMessageType::UserMessage, current_question });
    }
    if (current_response.size() > 0) {
        messages.push_back({ ChatMessageType::ResponseMessage, current_response });
    }
    return messages;
}

void SioyekBookmarkTextBrowser::update_text(QString new_text) {

    if (text_browser){
        int current_scroll_amount = text_browser->verticalScrollBar()->value();
        if (current_scroll_amount != last_set_scroll_amount) {
            follow_output = false;
        }

        //text_browser->setMarkdown(new_text);
        text_browser->update_text(new_text);


        if (follow_output) {
            int scroll_amount = text_browser->verticalScrollBar()->maximum();
            text_browser->verticalScrollBar()->setValue(scroll_amount);
            last_set_scroll_amount = scroll_amount;

        }
        else {
            text_browser->verticalScrollBar()->setValue(current_scroll_amount);
        }
    }
    else{
        QList<ChatMessage> chat_messages = get_chat_messages_from_bookmark_text(new_text);
        touch_text_browser->set_messages(chat_messages);
    }
}

void SioyekBookmarkTextBrowser::set_follow_output(bool val) {
    if (text_browser){
        last_set_scroll_amount = text_browser->verticalScrollBar()->value();
        follow_output = val;
    }
}

std::pair<int, int> SioyekChatTextBrowser::get_cursor_message_and_char_index(QPoint cursor_pos, int forced_message_index) {

    int y = margin - verticalScrollBar()->value();
    int msg_index = 0;

    for (auto [message_type, msg] : messages) {

        int box_height, box_width, box_x, inner_margin;
        QTextDocument* doc = get_doc_and_size_values_for_index(msg_index, &box_width, &box_height, &box_x, &inner_margin);

        if ((forced_message_index == -1) || (msg_index == forced_message_index)) {
            QRect message_rect(box_x, y, box_width, box_height);
            if (message_rect.contains(cursor_pos) || (forced_message_index != -1)) {
                QPointF localPos = cursor_pos - QPoint(box_x + inner_margin, y + inner_margin);
                int char_index = doc->documentLayout()->hitTest(localPos, Qt::FuzzyHit);
                return { msg_index, char_index };
            }
        }
        y += box_height + spacing;
        msg_index++;
    }
    return { -1, -1 };
}

void SioyekChatTextBrowser::mousePressEvent(QMouseEvent* mevent) {
    QPoint click_pos = mevent->pos();
    auto [message_index, char_index] = get_cursor_message_and_char_index(click_pos);
    if (message_index >= 0) {
        selection_message_index = message_index;
        selection_start_pos = char_index;
        selection_end_pos = char_index;
        is_selecting = true;
        viewport()->update();
    }
    QAbstractScrollArea::mousePressEvent(mevent);
    mevent->accept();
}

QTextDocument* SioyekChatTextBrowser::get_document_for_index(int index) {
    auto it = cached_documents.find(index);
    if (it != cached_documents.end()) {
        return it->second.get();
    }
    else {
        auto [message_type, message_text] = messages[index];
        std::unique_ptr<QTextDocument> doc = std::make_unique<QTextDocument>();
        doc->setDefaultFont(chat_font);
        doc->setMarkdown(message_text);
        if (message_type == ChatMessageType::ResponseMessage) {
            doc->setTextWidth(response_content_width);
        }
        else {
            float ideal_width = doc->idealWidth();
            float text_width = (ideal_width < user_content_width) ? ideal_width : user_content_width;
            doc->setTextWidth(text_width);
        }
        cached_documents[index] = std::move(doc);
        return cached_documents[index].get();
    }
    return nullptr;
}

QTextDocument* SioyekChatTextBrowser::get_doc_and_size_values_for_index(int index, int* out_box_width, int* out_box_height, int* out_box_x, int* out_inner_margin) {
    QTextDocument* doc = get_document_for_index(index);
    auto message_type = messages[index].message_type;
    if (message_type == ChatMessageType::UserMessage) {
        int required_width = static_cast<int>(doc->idealWidth()) + 2 * user_inner_margin;
        int box_width = std::min(user_box_width, required_width);

        *out_box_width = box_width;
        *out_inner_margin = user_inner_margin;
        *out_box_height = static_cast<int>(doc->size().height()) + 2 * user_inner_margin;
        *out_box_x = full_width - margin - box_width;
    }
    else {
        *out_box_width = response_content_width;
        *out_inner_margin = 0;
        *out_box_height = static_cast<int>(doc->size().height());
        *out_box_x = margin;
    }
    return doc;
}

QString SioyekChatTextBrowser::get_selected_text(){
    if (selection_message_index == -1) {
        return "";
    }
    else {
        QTextDocument* doc = get_document_for_index(selection_message_index);
        QTextCursor cursor(doc);
        cursor.setPosition(selection_start_pos);
        cursor.setPosition(selection_end_pos, QTextCursor::KeepAnchor);
        return cursor.selectedText();
    }
}

void SioyekChatTextBrowser::mouseMoveEvent(QMouseEvent* mevent) {
    QPoint mouse_pos = mevent->pos();

    int last_selection_end = selection_end_pos;
    if (is_selecting && selection_message_index != -1) {

        auto [message_index, char_index] = get_cursor_message_and_char_index(mouse_pos, selection_message_index);
        if (message_index >= 0 && message_index == selection_message_index) {
            selection_end_pos = char_index;
            viewport()->update();
        }
    }
    QAbstractScrollArea::mouseMoveEvent(mevent);
    mevent->accept();
}

void SioyekChatTextBrowser::mouseReleaseEvent(QMouseEvent* mevent) {
    // Check if a markdown link was clicked.
    QPoint click_pos = mevent->pos();

    int y = margin - verticalScrollBar()->value();

    int index = 0;
    for (auto [message_type, msg] : messages) {

        int box_height, box_width, box_x, inner_margin;
        QTextDocument* doc = get_doc_and_size_values_for_index(index, &box_width, &box_height, &box_x, &inner_margin);

        QSizeF docSize = doc->size();
        QRect message_rect(box_x, y, box_width, box_height);
        if (message_rect.contains(click_pos)) {
            QPointF localPos = click_pos - QPoint(box_x + inner_margin, y + inner_margin);
            QString anchor = doc->documentLayout()->anchorAt(localPos);
            if (!anchor.isEmpty()) {
                emit anchorClicked(anchor);
                break;
            }
        }
        y += box_height + spacing;
        index++;
    }

    mevent->accept();
}

void SioyekChatTextBrowser::mouseDoubleClickEvent(QMouseEvent* mevent) {
    QAbstractScrollArea::mouseDoubleClickEvent(mevent);
    mevent->accept();
}

SioyekChatTextBrowser::SioyekChatTextBrowser(QWidget* parent, QString text): QAbstractScrollArea(parent) {
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    update_text(text);
    chat_font = QFont(get_chat_font_face_name());
    chat_font.setPointSize(get_chat_font_size());
}


void SioyekChatTextBrowser::update_text(QString new_text) {
    // messages.clear();
    cached_documents.clear();
    messages = get_chat_messages_from_bookmark_text(new_text);
}

void SioyekChatTextBrowser::paintEvent(QPaintEvent* event) {
    QColor user_background_color = qconvert_color3(CHAT_WINDOW_USER_MESSAGE_BACKGROUND_COLOR, ColorPalette::Normal);
    QColor background_color = qconvert_color3(CHAT_WINDOW_BACKGROUND_COLOR, ColorPalette::Normal);
    QColor text_color = qconvert_color3(CHAT_WINDOW_TEXT_COLOR, ColorPalette::Normal);
    QColor user_text_color = qconvert_color3(CHAT_WINDOW_USER_TEXT_COLOR, ColorPalette::Normal);

    QPainter painter(viewport());
    painter.fillRect(rect(), background_color);
    painter.translate(0, -verticalScrollBar()->value());

    int y = margin;
    int msg_index = 0;

    QColor text_highlight_color = qconvert_color3(DEFAULT_TEXT_HIGHLIGHT_COLOR, ColorPalette::Normal);

    for (auto [message_type, msg] : messages) {

        int box_height, box_width, box_x, inner_margin;
        QTextDocument* doc = get_doc_and_size_values_for_index(msg_index, &box_width, &box_height, &box_x, &inner_margin);

        QRect messageRect(box_x, y, box_width, box_height);

        // draw background for user messages.
        if (message_type == ChatMessageType::UserMessage) {
            painter.save();
            painter.setRenderHint(QPainter::Antialiasing);
            painter.setBrush(user_background_color);
            painter.setPen(Qt::NoPen);
            int radius = painter.font().pointSize();
            painter.drawRoundedRect(messageRect, radius, radius);
            painter.restore();
        }

        painter.save();
        painter.translate(box_x + inner_margin, y + inner_margin);

        QAbstractTextDocumentLayout::PaintContext ctx;
        if (message_type == ChatMessageType::UserMessage) {
            ctx.palette.setColor(QPalette::Text, user_text_color);
            ctx.palette.setColor(QPalette::Base, user_background_color);
        }
        else {
            ctx.palette.setColor(QPalette::Text, text_color);
            ctx.palette.setColor(QPalette::Base, background_color);
        }
        // if this is the message being selected and a range exists, add the selection.
        if (msg_index == selection_message_index && selection_start_pos != selection_end_pos) {
            QTextLayout::FormatRange selection;
            selection.start = qMin(selection_start_pos, selection_end_pos);
            selection.length = qAbs(selection_end_pos - selection_start_pos);
            QTextCharFormat fmt;
            fmt.setBackground(text_highlight_color);
            fmt.setForeground(Qt::black);
            selection.format = fmt;
            QAbstractTextDocumentLayout::Selection selectionRange;
            selectionRange.cursor = QTextCursor(doc);
            selectionRange.cursor.setPosition(selection.start);
            selectionRange.cursor.setPosition(selection.start + selection.length, QTextCursor::KeepAnchor);
            selectionRange.format = fmt;
            ctx.selections.append(selectionRange);
        }

        doc->documentLayout()->draw(&painter, ctx);
        painter.restore();

        y += box_height + spacing;
        ++msg_index;
    }
    update_scrollbar(y);
}

void SioyekChatTextBrowser::resizeEvent(QResizeEvent* event) {
    cached_documents.clear();
    QAbstractScrollArea::resizeEvent(event);

    full_width = viewport()->width();
    response_content_width = full_width - 2 * margin;
    user_box_width = static_cast<int>(0.7 * full_width);
    user_content_width = user_box_width - 2 * user_inner_margin;

}

void SioyekChatTextBrowser::update_scrollbar(int content_height) {
    verticalScrollBar()->setPageStep(viewport()->height());
    verticalScrollBar()->setRange(0, content_height - viewport()->height());
}

void SioyekChatTextBrowser::wheelEvent(QWheelEvent* event) {
    int delta = event->angleDelta().y();

    int step = delta / 8 * verticalScrollBar()->singleStep() * 2;

    verticalScrollBar()->setValue(verticalScrollBar()->value() - step);
    event->accept();
}

void SioyekBookmarkTextBrowser::scroll_amount(int amount) {

    if (text_browser){
        int current_scroll_amount = text_browser->verticalScrollBar()->value();
        int step = text_browser->verticalScrollBar()->singleStep() * amount * 60;
        int new_scroll_amount = current_scroll_amount + step;
        text_browser->verticalScrollBar()->setValue(new_scroll_amount);
        last_set_scroll_amount = new_scroll_amount;
    }
}

void SioyekBookmarkTextBrowser::scroll_to_start(){
    if (text_browser){
        text_browser->verticalScrollBar()->setValue(0);
        last_set_scroll_amount = 0;
    }
}

void SioyekBookmarkTextBrowser::scroll_to_end() {
    if (text_browser){
        text_browser->verticalScrollBar()->setValue(text_browser->verticalScrollBar()->maximum());
        last_set_scroll_amount = text_browser->verticalScrollBar()->maximum();
    }

}

void SioyekBookmarkTextBrowser::set_pending(bool pending) {
    is_pending = pending;
    if (line_edit) {

        if (pending) {
            line_edit->setStyleSheet("QLineEdit{background-color: " + convert_float3_to_qcolor(CHAT_WINDOW_USER_MESSAGE_BACKGROUND_COLOR).lighter(150).name() + "; color: " + convert_float3_to_qcolor(CHAT_WINDOW_USER_TEXT_COLOR).lighter(150).name() + "; border-radius: 0px; padding: 10px;}");
        }
        else {
            line_edit->setStyleSheet("QLineEdit{background-color: " + convert_float3_to_qcolor(CHAT_WINDOW_USER_MESSAGE_BACKGROUND_COLOR).name() + "; color: " + convert_float3_to_qcolor(CHAT_WINDOW_USER_TEXT_COLOR).name() + "; border-radius: 0px; padding: 10px;}");
        }
    }
    if (touch_text_browser){
        touch_text_browser->set_pending(is_pending);
    }
}

SioyekBookmarkTextBrowser::~SioyekBookmarkTextBrowser() {
    if (is_bookmark_pending) {
        main_widget->doc()->undo_pending_bookmark(bookmark_uuid.toStdString());
    }
}

QRect SioyekBookmarkTextBrowser::get_prefered_rect(QRect parent_rect){
    return get_bookmark_chat_prefered_rect(parent_rect);
}

QLineEdit* SioyekBookmarkTextBrowser::get_line_edit(){
    return dynamic_cast<QLineEdit*>(line_edit);
}
