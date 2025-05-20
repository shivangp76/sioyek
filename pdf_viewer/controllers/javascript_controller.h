#pragma once

#include <vector>
#include <QString>
#include <QJSValue>

#include "controllers/base_controller.h"


class QJSEngine;

class JavascriptController : public BaseController {
private:
    QJSEngine* sync_js_engine = nullptr;
    std::vector<QJSEngine*> available_async_engines;

    std::mutex available_engine_mutex;
    int num_js_engines = 0;
    QString async_utility_code = "";
public:
    JavascriptController(MainWidget* parent);
    ~JavascriptController();
    QJSEngine* take_js_engine(bool async);
    void release_async_js_engine(QJSEngine* engine);
    QJSValue export_javascript_api(QJSEngine& engine, bool is_async);
    void add_async_utility_code(QString str);
    void run_javascript_command(std::wstring javascript_code, std::optional<std::wstring> entry_point, bool is_async);
    void run_startup_js(bool first_run=false);
    void call_async_js_function_with_args(const QString& code, QJsonArray args);
    void call_js_function_with_bookmark_arg_with_uuid(const QString& function_name, const std::string& uuid);
    void call_js_function_with_highlight_arg_with_uuid(const QString& function_name, const std::string& uuid);
    void call_js_function_with_portal_arg_with_uuid(const QString& function_name, const std::string& uuid);
};
