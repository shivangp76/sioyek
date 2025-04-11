#pragma once

#include <QWidget>
#include <QQuickWidget>
#include <QQmlContext>
#include <QQuickItem>
#include <QStringList>
#include <QStringListModel>
#include <QSortFilterProxyModel>
#include "mysortfilterproxymodel.h"
#include "config.h"


class Config;
class ConfigManager;
//class MySortFilterProxyModel;
class MainWidget;

class ConfigModel : public QAbstractTableModel {

private:
    std::vector<Config*>* configs;

public:
    explicit ConfigModel(std::vector<Config*>* configs, QObject* parent = nullptr);
    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    int columnCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const;
};

class TouchConfigMenu : public QWidget {
    Q_OBJECT
public:
    //TouchConfigMenu(std::vector<Config>* configs, QWidget* parent = nullptr);
    TouchConfigMenu(bool fuzzy, MainWidget* main_widget);
    // void resizeEvent(QResizeEvent* resize_event) override;
    Q_INVOKABLE QRect get_prefered_rect(QRect parent_rect);

public slots:
    void handleColor3ConfigChanged(QString config_name, qreal r, qreal g, qreal b);
    void handleColor4ConfigChanged(QString config_name, qreal r, qreal g, qreal b, qreal a);
    void handleBoolConfigChanged(QString config_name, bool new_value);
    void handleFloatConfigChanged(QString config_name, qreal new_value);
    void handleIntConfigChanged(QString config_name, int new_value);
    void handleTextConfigChanged(QString config_name, QString new_value);
    void handleSetConfigPressed(QString config_name);
    void handleSaveButtonClicked();
    //void handlePressAndHold(QString value, int index);
    //void handleDelete(QString value, int index);

signals:
    //void itemSelected(QString value, int index);
    //void itemPressAndHold(QString value, int index);
    //void itemDeleted(QString value, int index);

private:
    QQuickWidget* quick_widget = nullptr;
    ConfigManager* config_manager;
    //QStringList items;
    //QStringListModel model;
    MainWidget* main_widget;
    ConfigModel config_model;
    MySortFilterProxyModel* proxy_model;

};
