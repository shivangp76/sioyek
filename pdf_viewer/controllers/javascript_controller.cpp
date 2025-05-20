#include <QJSEngine>
#include <QFile>

#include "controllers/javascript_controller.h"
#include "main_widget.h"
#include "commands/base_commands.h"

extern Path sioyek_js_path;

JavascriptController::JavascriptController(MainWidget* parent) : BaseController(parent) {
}

JavascriptController::~JavascriptController(){
    if (sync_js_engine != nullptr) {
        sync_js_engine->collectGarbage();
        delete sync_js_engine;
    }
    for (int i = 0; i < available_async_engines.size(); i++) {
        available_async_engines[i]->collectGarbage();
        delete available_async_engines[i];
    }
}

QJSEngine* JavascriptController::take_js_engine(bool async) {
    //std::lock_guard guard(available_engine_mutex);
    if (!async) {
        if (sync_js_engine != nullptr) {
            return sync_js_engine;
        }

        auto js_engine = new QJSEngine();
        js_engine->installExtensions(QJSEngine::ConsoleExtension);

        QJSValue sioyek_object = js_engine->newQObject(mw);
        js_engine->setObjectOwnership(mw, QJSEngine::CppOwnership);

        js_engine->globalObject().setProperty("sioyek_api", sioyek_object);
        js_engine->globalObject().setProperty("window", sioyek_object);
        export_javascript_api(*js_engine, false);
        sync_js_engine = js_engine;
        return sync_js_engine;

    }
    available_engine_mutex.lock();

    while (num_js_engines > 4 && (available_async_engines.size() == 0)) {
        available_engine_mutex.unlock();
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        available_engine_mutex.lock();
    }

    if (available_async_engines.size() > 0) {
        auto res = available_async_engines.back();
        available_async_engines.pop_back();
        available_engine_mutex.unlock();
        return res;
    }

    auto js_engine = new QJSEngine();
    js_engine->installExtensions(QJSEngine::ConsoleExtension);
    num_js_engines++;

    available_engine_mutex.unlock();

    QJSValue sioyek_object = js_engine->newQObject(mw);
    js_engine->setObjectOwnership(mw, QJSEngine::CppOwnership);

    js_engine->globalObject().setProperty("sioyek_api", sioyek_object);
    js_engine->globalObject().setProperty("window", sioyek_object);
    export_javascript_api(*js_engine, true);
    return js_engine;
}

void JavascriptController::release_async_js_engine(QJSEngine* engine) {
    std::lock_guard guard(available_engine_mutex);
    available_async_engines.push_back(engine);
}

QJSValue JavascriptController::export_javascript_api(QJSEngine& engine, bool is_async) {

    QJSValue res = engine.newObject();
    engine.globalObject().setProperty("sioyek", res);

    QStringList command_names = mw->command_manager->get_all_command_names();
    QString all_command_eval_string = "[";
    for (int i = 0; i < command_names.size(); i++) {
        if (command_names[i] == "import") continue;
        all_command_eval_string += "\"" + command_names[i] + "\"";

        if (i < command_names.size() - 1) {
            all_command_eval_string += ",";
        }

    }
    all_command_eval_string += "]";
    engine.evaluate("__all_command_names=" + all_command_eval_string);

    //engine.globalObject().setProperty("__all_command_names", command_names);


    if (is_async) {
        QJSValue res = engine.evaluate(async_utility_code + "\n" + "\
            for (let i = 0; i < __all_command_names.length; i++){\
                let cname = __all_command_names[i];\
                sioyek[cname] = (...args)=>{\
                    let arg_strings = args.map((arg) => {return '' + arg;});\
                    return sioyek_api.run_macro_on_main_thread(cname, arg_strings);\
                };\
                sioyek['$' + cname] = (...args)=>{\
                    let arg_strings = args.map((arg) => {return '' + arg;});\
                    return sioyek_api.run_macro_on_main_thread(cname, arg_strings, false);\
                };\
            }\
        ");
        if (res.isError()) {
            qDebug() << "Javascript error: " << res.toString();
        }
    }
    else {
        QJSValue res1 = engine.evaluate("__sioyek_keybind_function_index=0;\
                        function addAsyncUtilityCode(codeString){\
                            sioyek_api.add_async_utility_code(codeString);\
                        }\
                        function addHook(eventType, codeString){\
                            sioyek_api.register_hook_function(eventType, codeString);\
                        }\
                        function addKeybind(keybind, callable, command_name){\
                            let backtrace = __get_stacktrace();\
                            let line = new Error().stack;\
                            if (typeof(callable) == 'string'){\
                                sioyek_api.register_string_keybind(keybind, callable, backtrace[0], backtrace[1]);\
                            }\
                            else{\
                                let name = '__sioyek_keybind_function_' + __sioyek_keybind_function_index;\
                                __sioyek_keybind_function_index++;\
                                this[name] = callable;\
                                sioyek_api.register_function_keybind(keybind, name, command_name, backtrace[0], backtrace[1]);\
                                return name;\
                            }\
                        }\
                        function addKeybindAsync(keybind, callable, command_name){\
                            let backtrace = __get_stacktrace();\
                            if (typeof(callable) !== 'string'){sioyek_api.report_js_error('Error in ' + backtrace[0] + ':' + backtrace[1] + ': async function should be a string, if you are passing a raw function, you can convert it to a string by surrounding it with `.', backtrace[0], backtrace[1]); return;}\
                            sioyek_api.register_function_keybind_async(keybind, callable, command_name, backtrace[0], backtrace[1]);\
                        }\
                        function addCommandAsync(command_name, callable){\
                            addKeybindAsync('', callable, command_name);\
                        }\
                        function addCommand(command_name, callable){\
                            addKeybind('', callable, command_name);\
                        }\
                        ");
        QJSValue res2 = engine.evaluate("\
            for (let i = 0; i < __all_command_names.length; i++){\
                let cname = __all_command_names[i];\
                sioyek[cname] = (...args)=>{\
                    let arg_strings = args.map((arg) => {return '' + arg;});\
                    let args_string = arg_strings.join(',');\
                    if (args_string.length > 0) {return sioyek_api.execute_macro_sync(cname, arg_strings);}\
                    else {return sioyek_api.execute_macro_sync(cname);}\
                    console.log('happened');\
                }\
            }\
        ");
        if (res1.isError()) {
            qDebug() << "Javascript error: " << res1.toString();
        }
        if (res2.isError()) {
            qDebug() << "Javascript error: " << res2.toString();
        }
    }


    return res;
}

void JavascriptController::add_async_utility_code(QString str) {
    async_utility_code += str;
}

void JavascriptController::run_javascript_command(std::wstring javascript_code, std::optional<std::wstring> entry_point, bool is_async){

    QString content = QString::fromStdWString(javascript_code);
    if (entry_point.has_value()) {
        content += "\n" + QString::fromStdWString(entry_point.value()) + "()";
    }

    if (is_async) {
        std::thread ext_thread = std::thread([&, content]() {
            QJSEngine* engine = take_js_engine(true);
            auto res = engine->evaluate(content);
            release_async_js_engine(engine);
            });
        ext_thread.detach();
    }
    else {
        QJSEngine* engine = take_js_engine(false);
        QStringList stack_trace;
        auto res = engine->evaluate(content, QString(), 1, &stack_trace);
        //release_js_engine(engine);
        if (res.isError()){
            qDebug() << res.toString();
        }
        if (stack_trace.size() > 0) {
            for (auto line : stack_trace) {
                qDebug() << line;
            }
        }
    }

}

void JavascriptController::run_startup_js(bool first_run) {
#ifndef SIOYEK_MOBILE
    QString js_path_qstring = QString::fromStdWString(sioyek_js_path.get_path());
    QFile sioyek_js(js_path_qstring);

    if (sioyek_js.exists()) {
        sioyek_js.open(QIODeviceBase::ReadOnly);
        auto js_engine = take_js_engine(false);
        QString prelude = "let __first_run = %{FIRST_RUN};\n\
            if (!__first_run){addKeybind = ()=>{}; addKeybindAsync = ()=>{}}\n\
            function include(path){\
                let content = sioyek_api.read_file(path);\
                eval(content);\
            }\
            function __get_stacktrace(){\n\
                let lines = new Error().stack.split('\\n');\n\
                let line = lines[lines.length-1];\n\
                let lineNumber =  (0 + line.split(':')[1]) - %{PRELUDE_LINES};\n\
                return ['"+ js_path_qstring + "', lineNumber];\
            }\
            ";
        int n_lines = prelude.count('\n');
        prelude = prelude.replace("%{PRELUDE_LINES}", QString::number(n_lines));
        prelude = prelude.replace("%{FIRST_RUN}", first_run ? "true" : "false");
        QString file_data = QString::fromUtf8(sioyek_js.readAll());
        js_engine->evaluate(prelude + file_data);
        sioyek_js.close();
    }
#endif
}

void JavascriptController::call_async_js_function_with_args(const QString& code, QJsonArray args){
    std::thread ext_thread = std::thread([&, code, args]() {
        QJSEngine* engine = take_js_engine(true);
        //auto jsargs = engine->toScriptValue(args);
        auto func = engine->evaluate(code);
        if (func.isError()) {
            qDebug() << "error in js function: " << func.toString();
            return;
        }
        QJSValueList js_args;
        for (auto arg : args) {
            if (arg.isArray()) {
                js_args.push_back(engine->toScriptValue(arg.toArray()));
            }
            else if (arg.isBool()) {
                js_args.push_back(engine->toScriptValue(arg.toBool()));
            }
            else if (arg.isDouble()) {
                js_args.push_back(engine->toScriptValue(arg.toDouble()));
            }
            else if (arg.isObject()) {
                js_args.push_back(engine->toScriptValue(arg.toObject()));
            }
            else if (arg.isString()) {
                js_args.push_back(engine->toScriptValue(arg.toString()));
            }
        }
        QJSValue result = func.call(js_args);

        if (result.isError()) {
            qDebug() << "error in js function: " << result.toString();
        }

        release_async_js_engine(engine);
        });
    ext_thread.detach();

}

void JavascriptController::call_js_function_with_bookmark_arg_with_uuid(const QString& function_name, const std::string& uuid) {
    int bookmark_index = doc()->get_bookmark_index_with_uuid(uuid);
    if (bookmark_index >= 0 && bookmark_index < doc()->get_bookmarks().size()) {
        BookMark bookmark = doc()->get_bookmarks()[bookmark_index];
        call_async_js_function_with_args(function_name, QJsonArray() << bookmark.to_json(""));
    }
}

void JavascriptController::call_js_function_with_highlight_arg_with_uuid(const QString& function_name, const std::string& uuid) {
    int highlight_index = doc()->get_highlight_index_with_uuid(uuid);
    if (highlight_index >= 0 && highlight_index < doc()->get_highlights().size()) {
        Highlight highlight = doc()->get_highlights()[highlight_index];

        call_async_js_function_with_args(function_name, QJsonArray() << highlight.to_json(""));
    }
}

void JavascriptController::call_js_function_with_portal_arg_with_uuid(const QString& function_name, const std::string& uuid) {
    int portal_index = doc()->get_portal_index_with_uuid(uuid);
    if (portal_index >= 0 && portal_index < doc()->get_portals().size()) {
        Portal portal = doc()->get_portals()[portal_index];
        call_async_js_function_with_args(function_name, QJsonArray() << portal.to_json(""));
    }
}
