#include "touchui/TouchConfigMenu.h"
#include <QVariant>
#include "ui.h"
#include "main_widget.h"
#include "config.h"


TouchConfigMenu::TouchConfigMenu(bool fuzzy, MainWidget* main_widget) :
    QWidget(main_widget),
    config_model(main_widget->config_manager->get_configs_ptr()),
    config_manager(main_widget->config_manager),
    main_widget(main_widget)
{

    setAttribute(Qt::WA_NoMousePropagation);

    proxy_model = new MySortFilterProxyModel(fuzzy, false);
    proxy_model->setParent(this);

    proxy_model->setSourceModel(&config_model);
    proxy_model->setFilterKeyColumn(1);

    //    quick_widget = new QQuickWidget(QUrl("qrc:/pdf_viewer/touchui/TouchSlider.qml"), this);
    quick_widget = new QQuickWidget(this);
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->addWidget(quick_widget);
    setLayout(layout);

    quick_widget->setResizeMode(QQuickWidget::ResizeMode::SizeRootObjectToView);
    quick_widget->setClearColor(Qt::transparent);
    //quick_widget->setAttribute(Qt::WA_AlwaysStackOnTop);
    //quick_widget->setClearColor(Qt::transparent);

    quick_widget->rootContext()->setContextProperty("_model", QVariant::fromValue(proxy_model));
    quick_widget->rootContext()->setContextProperty("_deletable", QVariant::fromValue(proxy_model));

    quick_widget->setSource(QUrl("qrc:/pdf_viewer/touchui/TouchConfigMenu.qml"));


    QObject::connect(dynamic_cast<QObject*>(quick_widget->rootObject()), SIGNAL(color3ConfigChanged(QString, qreal, qreal, qreal)), this, SLOT(handleColor3ConfigChanged(QString, qreal, qreal, qreal)));
    QObject::connect(dynamic_cast<QObject*>(quick_widget->rootObject()), SIGNAL(color4ConfigChanged(QString, qreal, qreal, qreal, qreal)), this, SLOT(handleColor4ConfigChanged(QString, qreal, qreal, qreal, qreal)));
    QObject::connect(dynamic_cast<QObject*>(quick_widget->rootObject()),
        SIGNAL(boolConfigChanged(QString, bool)),
        this,
        SLOT(handleBoolConfigChanged(QString, bool)));

    QObject::connect(dynamic_cast<QObject*>(quick_widget->rootObject()),
        SIGNAL(floatConfigChanged(QString, qreal)),
        this,
        SLOT(handleFloatConfigChanged(QString, qreal)));
    QObject::connect(dynamic_cast<QObject*>(quick_widget->rootObject()),
        SIGNAL(textConfigChanged(QString, QString)),
        this,
        SLOT(handleTextConfigChanged(QString, QString)));

    QObject::connect(dynamic_cast<QObject*>(quick_widget->rootObject()),
        SIGNAL(onSetConfigPressed(QString)),
        this,
        SLOT(handleSetConfigPressed(QString)));

    QObject::connect(dynamic_cast<QObject*>(quick_widget->rootObject()),
        SIGNAL(onSaveButtonClicked()),
        this,
        SLOT(handleSaveButtonClicked()));
    //QObject::connect(dynamic_cast<QObject*>(quick_widget->rootObject()), SIGNAL(itemPressAndHold(QString, int)), this, SLOT(handlePressAndHold(QString, int)));
    //QObject::connect(dynamic_cast<QObject*>(quick_widget->rootObject()), SIGNAL(itemDeleted(QString, int)), this, SLOT(handleDelete(QString, int)));

}

void TouchConfigMenu::handleBoolConfigChanged(QString config_name, bool new_value) {
    Config* config = config_manager->get_mut_config_with_name(config_name.toStdWString());
    assert(config->config_type == ConfigType::Bool);
    *((bool*)config->value) = new_value;
}

void TouchConfigMenu::handleFloatConfigChanged(QString config_name, qreal new_value) {
    Config* config = config_manager->get_mut_config_with_name(config_name.toStdWString());
    assert(config->config_type == ConfigType::Float);
    *((float*)config->value) = new_value;
    main_widget->invalidate_render();
}

void TouchConfigMenu::handleTextConfigChanged(QString config_name, QString new_value) {
    Config* config = config_manager->get_mut_config_with_name(config_name.toStdWString());
    assert(config->config_type == ConfigType::String);
    *((std::wstring*)config->value) = new_value.toStdWString();
}

void TouchConfigMenu::handleSetConfigPressed(QString config_name) {
    //command_manager->execute_command(CommandType::SetConfig, config_name.toStdWString());
    main_widget->execute_macro_if_enabled((L"show_touch_ui_for_config('" + config_name.toStdWString() + L"')"));
    //main_widget->show_touch
    //auto command = main_widget->command_manager->get_command_with_name(main_widget, (QString("setconfig_") + config_name).toStdString());
    //main_widget->handle_command_types(std::move(command), 0);
}

void TouchConfigMenu::handleIntConfigChanged(QString config_name, int new_value) {
    Config* config = config_manager->get_mut_config_with_name(config_name.toStdWString());
    assert(config->config_type == ConfigType::Int);
    *((int*)config->value) = new_value;
    main_widget->invalidate_render();
}

void TouchConfigMenu::handleColor3ConfigChanged(QString config_name, qreal r, qreal g, qreal b) {
    Config* config = config_manager->get_mut_config_with_name(config_name.toStdWString());
    assert(config->config_type == ConfigType::Color3);
    *((float*)config->value + 0) = r;
    *((float*)config->value + 1) = g;
    *((float*)config->value + 2) = b;
    //convert_qcolor_to_float3(QColor(r, g, b), (float*)config->value);
}

void TouchConfigMenu::handleColor4ConfigChanged(QString config_name, qreal r, qreal g, qreal b, qreal a) {
    Config* config = config_manager->get_mut_config_with_name(config_name.toStdWString());
    assert(config->config_type == ConfigType::Color4);
    *((float*)config->value + 0) = r;
    *((float*)config->value + 1) = g;
    *((float*)config->value + 2) = b;
    *((float*)config->value + 3) = a;
}

void TouchConfigMenu::handleSaveButtonClicked() {
    main_widget->persist_config();
    main_widget->pop_current_widget();
}

//
//void TouchListView::handleDelete(QString val, int index) {
//    int source_index = proxy_model.mapToSource( proxy_model.index(index, 0)).row();
//    emit itemDeleted(val, source_index);
//}
//
//void TouchListView::handlePressAndHold(QString val, int index) {
//    int source_index = proxy_model.mapToSource( proxy_model.index(index, 0)).row();
//    emit itemPressAndHold(val, source_index);
//}


QRect TouchConfigMenu::get_prefered_rect(QRect parent_rect){
    int parent_width = parent_rect.width();
    int parent_height = parent_rect.height();
    int w = parent_width * 0.8f;
    int h = parent_height * 0.8f;

    return QRect(parent_width * 0.1f, parent_height * 0.1f, w, h);
}


ConfigModel::ConfigModel(std::vector<Config*>* configs, QObject* parent) : QAbstractTableModel(parent), configs(configs) {
}

int ConfigModel::rowCount(const QModelIndex& parent) const {
    return configs->size();
}

int ConfigModel::columnCount(const QModelIndex& parent) const {
    return 3;
}
QVariant ConfigModel::data(const QModelIndex& index, int role) const {
    if (role == Qt::DisplayRole) {
        int col = index.column();
        int row = index.row();
        if (col < 0 || row < 0) {
            //return QAbstractTableModel::data(index, role);
            return QVariant::fromValue(QString(""));
        }

        const Config* conf = (*configs)[row];
        ConfigType config_type = conf->config_type;
        std::wstring config_name = conf->name;
        if (col == 0) {
            return QVariant::fromValue(QString::fromStdWString(conf->get_type_string()));
        }
        if (col == 1) {
            return QVariant::fromValue(QString::fromStdWString(config_name));
        }
        if (col == 2) {
            //std::wstringstream config_serialized;
            //conf->serialize(conf->value, config_serialized);
            //QVariant::fromValue(QString::fromStdWString(config_serialized.str()));
            if (config_type == ConfigType::Bool) {
                return QVariant::fromValue(*(bool*)conf->value);
            }

            if (config_type == ConfigType::Float) {
                QList<float> vals;
                FloatExtras extras = std::get<FloatExtras>(conf->extras);
                vals << *(float*)conf->value;
                vals << extras.min_val << extras.max_val;
                return QVariant::fromValue(vals);
            }
            if (config_type == ConfigType::Int) {
                QList<int> vals;
                IntExtras extras = std::get<IntExtras>(conf->extras);
                vals << *(int*)conf->value;
                vals << extras.min_val << extras.max_val;
                return QVariant::fromValue(vals);
            }

            if (config_type == ConfigType::Enum) {
                int index = *(int*)conf->value;
                return QVariant::fromValue(QString::fromStdWString(std::get<EnumExtras>(conf->extras).possible_values[index]));
            }
            if ((config_type == ConfigType::String) || (config_type == ConfigType::Macro) || (config_type == ConfigType::FilePath) || (config_type == ConfigType::FolderPath)) {
                //QColor::from
                return QVariant::fromValue(QString::fromStdWString(*(std::wstring*)(conf->value)));
            }
            if (config_type == ConfigType::Color3) {
                //QColor::from
                int out_rgb[3];
                convert_color3((float*)conf->value, out_rgb);
                return QVariant::fromValue(QColor(out_rgb[0], out_rgb[1], out_rgb[2]));
            }

            if (config_type == ConfigType::Color4) {
                //QColor::from
                int out_rgb[4];
                convert_color4((float*)conf->value, out_rgb);
                return QVariant::fromValue(QColor(out_rgb[0], out_rgb[1], out_rgb[2], out_rgb[3]));
            }

            //if (config_type == ConfigType::String) {
            //	//QColor::from
            //	return QVariant::fromValue(QString::fromStdWString(*(std::wstring*)conf->value));
            //}
            return QVariant::fromValue(QString(""));
        }

        //conf->name

    }
    return QVariant::fromValue(QString(""));
}
