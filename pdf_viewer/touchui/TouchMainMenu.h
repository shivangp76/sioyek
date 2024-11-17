#pragma once

#include <QWidget>
#include <QQuickWidget>
#include <QQmlContext>
#include <QQuickItem>


class TouchMainMenu : public QWidget {
    Q_OBJECT
public:
    TouchMainMenu(bool fit_mode,
        bool portaling,
        bool fullscreen,
        bool ruler,
        bool speaking,
        bool locked,
        int current_colorscheme_index,
        bool is_logged_in,
        bool is_current_document_synced,
        float current_brightness,
        QWidget* parent);
    void resizeEvent(QResizeEvent* resize_event) override;

public slots:
    void handleSelectText();
    void handleGotoPage();
    void handleToc();
    void handleSearch();
    void handleFullscreen();
    void handleBookmarks();
    void handleHighlights();
    void handleRuler();
    void handleLightColorscheme();
    void handleDarkColorscheme();
    void handleCustomColorscheme();
    void handleOpenPrevDoc();
    void handleOpenNewDoc();
    void handleCommands();
    void handleSettings();
    void handleAddBookmark();
    void handlePortal();
    void handleDeletePortal();
    void handleGlobalBookmarks();
    void handleGlobalHighlights();
    void handleTTS();
    void handleHorizontalLock();
    void handleFitToPageWidth();
    void handleDrawingMode();
    void handleDownloadPaper();
    void handleHint();
    void handleLogin();
    void handleLogout();
    void handleSync();
    void handleRefresh();
    void handleBrightnessChanged(double);

signals:
    void selectTextClicked();
    void gotoPageClicked();
    void tocClicked();
    void searchClicked();
    void fullscreenClicked();
    void bookmarksClicked();
    void highlightsClicked();
    void rulerModeClicked();
    void lightColorschemeClicked();
    void darkColorschemeClicked();
    void customColorschemeClicked();
    void openPrevDocClicked();
    void openNewDocClicked();
    void commandsClicked();
    void settingsClicked();
    void addBookmarkClicked();
    void portalClicked();
    void deletePortalClicked();
    void globalBookmarksClicked();
    void globalHighlightsClicked();
    void ttsClicked();
    void horizontalLockClicked();
    void fitToPageWidthClicked();
    void drawingModeClicked();
    void downloadPaperClicked();
    void hintClicked();
    void loginClicked();
    void logoutClicked();
    void syncClicked();
    void refreshClicked();
    void brightnessChanged(double);
private:
    QQuickWidget* quick_widget = nullptr;

};
