#include "network_manager.h"

#include <qurlquery.h>
#include <qnetworkreply.h>
#include <qjsondocument.h>
#include <qfile.h>
#include <qdir.h>
#include <qhttpmultipart.h>
#include <qtimer.h>

#include "utils.h"
#include "path.h"
#include "checksum.h"
#include "database.h"
#include "document.h"

extern std::string APPLICATION_VERSION;
extern Path sioyek_access_token_path;
extern Path cached_tts_path;
extern Path standard_data_path;
extern std::wstring PAPERS_FOLDER_PATH;
extern bool AUTOMATICALLY_DOWNLOAD_MATCHING_PAPER_NAME;

SioyekNetworkManager::SioyekNetworkManager(DatabaseManager* db_manager_, BackgroundTaskManager* task_manager, DocumentManager* doc_manager, QObject* parent) :
   db_manager(db_manager_), background_task_manager(task_manager) , document_manager(doc_manager) {
    last_document_location_upload_time = QDateTime::currentDateTime();
    QObject::connect(&network_manager, &QNetworkAccessManager::finished, [](QNetworkReply* reply) {
        reply->deleteLater();
        });
}

void SioyekNetworkManager::login(std::wstring username, std::wstring password) {
    QNetworkRequest req;
    req.setUrl(QUrl(QString::fromStdWString(SIOYEK_TOKEN_URL)));
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
    QUrlQuery params;
    params.addQueryItem("username", QString::fromStdWString(username));
    params.addQueryItem("password", QString::fromStdWString(password));
    QNetworkReply* reply = network_manager.post(req, params.query().toUtf8());

    reply->setProperty("sioyek_handled", true);

    status = ServerStatus::LoggingIn;
    QObject::connect(reply, &QNetworkReply::finished, [this, reply]() {
        if (handle_network_reply_if_error(reply, true)) {
            auto json_resp = QJsonDocument::fromJson(reply->readAll());
            ACCESS_TOKEN = json_resp.object()["access_token"].toString().toStdString();
            status = ServerStatus::LoggedIn;
            persist_access_token(ACCESS_TOKEN);
            handle_one_time_network_operations();
        }
        else {
            int status_code = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
            if (status_code == 401) {
                status = ServerStatus::InvalidCredentials;
            }
            else {
                status = ServerStatus::NotLoggedIn;
            }
        }
        });
}

bool SioyekNetworkManager::handle_network_reply_if_error(QNetworkReply* reply, bool show_message) {

    // returns true if there were no errors

    int status_code = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    if (status_code != 200) {

        auto data = reply->readAll();
        if (data.size() > 0) {
            auto json_resp = QJsonDocument::fromJson(data);
            if (!json_resp.isNull() && !json_resp.object().find("detail").value().isNull()) {
            //if (!json_resp.isNull() && !json_resp.object().find("detail").value().isNull()) {
                QString error_msg = json_resp.object()["detail"].toString();
                if (show_message) {
                    show_error_message(error_msg.toStdWString());
                }
            }
            else {
                if (show_message) {
                    QString error_msg = QString::fromUtf8(data);
                    show_error_message(error_msg.toStdWString());
                }
            }
        }
        else {
            if (status_code == 0) {
                status = ServerStatus::ServerOffline;
            }
            else {
                // handle error
            }
        }
        return false;
    }
    else {
        return true;
    }
}

void SioyekNetworkManager::persist_access_token(std::string access_token) {
    QFile access_token_file(QString::fromStdWString(sioyek_access_token_path.get_path()));
    if (access_token_file.open(QIODeviceBase::WriteOnly)) {
        access_token_file.write(QString::fromStdString(access_token).toUtf8());
        access_token_file.close();
    }
}

void SioyekNetworkManager::load_access_token() {
    QFile access_token_file(QString::fromStdWString(sioyek_access_token_path.get_path()));
    if (access_token_file.open(QIODeviceBase::ReadOnly)) {
        ACCESS_TOKEN = QString::fromUtf8(access_token_file.readAll()).toStdString();
        access_token_file.close();

        if (ACCESS_TOKEN.size() > 0) {
            // make sure the access token is still valid
            QUrl url(QString::fromStdWString(SIOYEK_ECHO_URL));

            QNetworkRequest req;
            authorize_request(&req);
            req.setUrl(url);
            auto reply = network_manager.get(req);
            reply->setProperty("sioyek_handled", true);
            //
            status = ServerStatus::LoggingIn;
            QObject::connect(reply, &QNetworkReply::finished, [this, reply]() {
                int status_code = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
                if (status_code == 401) {
                    ACCESS_TOKEN = "";
                    persist_access_token(ACCESS_TOKEN);
                    show_error_message(L"Access token was expired, please login again");
                }
                else {
                    status = ServerStatus::LoggedIn;
                    handle_one_time_network_operations();
                }
                });
        }
    }
}

void SioyekNetworkManager::authorize_request(QNetworkRequest* req) {
    req->setRawHeader("Authorization", ("Bearer " + QString::fromStdString(ACCESS_TOKEN)).toUtf8());
}

void SioyekNetworkManager::download_file_with_hash(QObject* parent, QString hash, std::function<void(QString)> fn) {
    QUrl url(QString::fromStdWString(SIOYEK_DOWNLOAD_FILE_WITH_HASH_PATH));
    QUrlQuery query;
    query.addQueryItem("hash", hash);
    url.setQuery(query);


    QNetworkRequest req;
    authorize_request(&req);
    req.setUrl(url);
    QNetworkReply* reply = network_manager.get(req);
    reply->setProperty("sioyek_handled", true);
    reply->setParent(parent);
    QObject::connect(reply, &QNetworkReply::finished, [this, reply, hash, fn=std::move(fn)]() {

        QString filename = reply->header(QNetworkRequest::ContentDispositionHeader).toString().mid(22);
        filename = filename.left(filename.size() - 1);
        QDir papers_dir(QString::fromStdWString(PAPERS_FOLDER_PATH));
        QString download_path = papers_dir.filePath(filename);
        QFileInfo download_file_info(download_path);
        if (!download_file_info.exists()) {
            QFile file(download_path);
            if (file.open(QIODeviceBase::WriteOnly)) {
                file.write(reply->readAll());
                file.close();
            }
            fn(download_path);
        }
        else {
            // if the current file in that location has the correct checksum, we just use that file
            // otherwise we must select a new, unique name

            std::string checksum = compute_checksum(download_file_info.filePath(), QCryptographicHash::Md5);
            if (checksum == hash.toStdString()) {
                fn(download_path);
            }
            else {
                QDir directory = download_file_info.dir();
                QString file_name = download_file_info.fileName();
                int dot_index = file_name.indexOf(".");
                if (dot_index > 0) {
                    QString prefix = file_name.left(dot_index);
                    QString suffix = file_name.mid(dot_index);
                    int index = 0;
                    QFileInfo new_path = QFileInfo(directory.filePath(prefix + QString::number(index) + suffix));
                    while (new_path.exists()) {
                        index++;
                        new_path = QFileInfo(directory.filePath(prefix + QString::number(index) + suffix));
                    }

                    QFile file(new_path.filePath());
                    if (file.open(QIODeviceBase::WriteOnly)) {
                        file.write(reply->readAll());
                        file.close();
                    }
                    fn(new_path.filePath());
                }
            }
        }
        });

}

QNetworkReply* SioyekNetworkManager::get_user_file_hash_set_reply() {
    QNetworkRequest req;
    authorize_request(&req);
    req.setUrl(QUrl(QString::fromStdWString(SIOYEK_USER_FILE_HASH_SET_URL)));
    QNetworkReply* reply = network_manager.get(req);
    reply->setProperty("sioyek_handled", true);
    return reply;
}

void SioyekNetworkManager::upload_file(QObject * parent, QString path, QString hash, std::function<void()> fn){
    QFile* file = new QFile(path);
    if (file->open(QIODevice::ReadOnly)) {
        QHttpMultiPart* parts = new QHttpMultiPart(QHttpMultiPart::FormDataType);

        QJsonDocument json_doc;
        QJsonObject root_object;
        root_object["sioyek_version"] = QString::fromStdString(APPLICATION_VERSION);
        root_object["file_checksum"] = hash;
        root_object["file_name"] = path;
        json_doc.setObject(root_object);


        QHttpPart data_part;
        data_part.setHeader(QNetworkRequest::ContentDispositionHeader, QVariant("form-data; name=\"sioyek_data\""));
        data_part.setBody(json_doc.toJson(QJsonDocument::JsonFormat::Compact));
        parts->append(data_part);

        QHttpPart file_part;
        file_part.setHeader(QNetworkRequest::ContentDispositionHeader, "form-data; name=\"file\"; filename=\"" + file->fileName() + "\"");
        file_part.setBodyDevice(file);
        parts->append(file_part);


        QNetworkRequest req;
        authorize_request(&req);
        req.setUrl(QUrl(QString::fromStdWString(SIOYEK_UPLOAD_URL)));
        //qDebug() << "boundary is:" << parts->boundary();
        //qDebug() << "file size  is:" << file->size();

        // DO NOT use the Qt's default boundary, it does not work
        parts->setBoundary(create_random_string().toUtf8());
        req.setHeader(QNetworkRequest::KnownHeaders::ContentTypeHeader, "multipart/form-data; boundary=" + parts->boundary());

        QNetworkReply* reply = network_manager.post(req, parts);
        reply->setProperty("sioyek_handled", true);
        reply->setParent(parent);

        QObject::connect(reply, &QNetworkReply::finished, [this, reply, fn=std::move(fn)]() {

            if (handle_network_reply_if_error(reply, true)) {
                update_user_files_hash_set();
                auto json_resp = QJsonDocument::fromJson(reply->readAll());
                if (json_resp["status"] != "OK" && json_resp["type"] == "incorrect_file_hash") {
                    //todo: update_current_document_checksum()

                    show_error_message(L"the file hash was incorrect");
                }
                else {
                    fn();
                }
            }
            });

        parts->setParent(reply);
    }
}

void SioyekNetworkManager::update_user_files_hash_set() {
    if (ACCESS_TOKEN.size() > 0) {
        QNetworkReply* reply = get_user_file_hash_set_reply();
        QObject::connect(reply, &QNetworkReply::finished, [this, reply]() {
            auto json_reply_ = get_network_json_reply(reply);
            if (json_reply_) {
                QJsonObject json_object = json_reply_->object();

                if (json_object["status"].toString() != "OK") return;

                SERVER_HASHES.clear();
                SERVER_DELETED_FILES.clear();
                for (auto server_hash : json_object["results"].toArray()) {
                    SERVER_HASHES.insert(server_hash.toString().toStdString());
                }
                for (auto server_deleted_file : json_object["deleted_files"].toArray()) {
                    SERVER_DELETED_FILES.insert(server_deleted_file.toString().toStdString());
                }
            }
            });
    }
}

std::optional<QJsonDocument> SioyekNetworkManager::get_network_json_reply(QNetworkReply* reply) {
    auto data = reply->readAll();
    if (data.size() > 0) {
        auto json_resp = QJsonDocument::fromJson(data);
        return json_resp;
    }
    return {};
}

QNetworkReply* SioyekNetworkManager::download_paper_with_name(QObject* parent, const std::wstring& name, PaperDownloadFinishedAction action, std::function<void(QNetworkReply*)> fn) {
    std::wstring download_name = name;
    if (name.size() > 0 && name[0] == ':') {
        download_name = name.substr(1, name.size() - 1);
    }

    QUrl url(QString::fromStdWString(SIOYEK_PAPER_URL_URL));
    QUrlQuery params;
    params.addQueryItem("paper_title", QString::fromStdWString(download_name));
    url.setQuery(params);

    //QUrl get_url = replace(
    //    "%{query}",
    //    QUrl::toPercentEncoding(QString::fromStdWString(download_name))
    //);

    //std::wstring get_url_string = get_url.toString().toStdWString();
    QNetworkRequest req;
    std::string user_agent_string = get_user_agent_string();
    req.setRawHeader("User-Agent", user_agent_string.c_str());
    req.setUrl(url);
    authorize_request(&req);
    auto reply = network_manager.get(req);
    reply->setProperty("sioyek_paper_name", QString::fromStdWString(name));
    reply->setProperty("sioyek_finish_action", get_paper_download_finish_action_string(action));
    QObject::connect(reply, &QNetworkReply::finished, [this, reply, parent, fn=std::move(fn)]() {

        int status_code = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        QString rep_url = reply->url().toString();

        if (status_code == 401) {
            show_error_message(L"This action requires login");
            return;
        }

        std::string answer = reply->readAll().toStdString();
        QByteArray json_data = QByteArray::fromStdString(answer);
        QJsonDocument json_doc = QJsonDocument::fromJson(json_data);

        if (json_doc["status"].toString() != "OK") return;

        std::wstring paper_name = reply->property("sioyek_paper_name").toString().toStdWString();
        PaperDownloadFinishedAction download_finish_action = get_paper_download_action_from_string(
            reply->property("sioyek_finish_action").toString());

        QStringList paper_urls;
        QStringList paper_titles;

        for (auto tuple : json_doc.object()["results"].toArray()) {
            auto title_url_tuple = tuple.toArray();
            paper_titles.append(title_url_tuple[0].toString());
            paper_urls.append(title_url_tuple[1].toString());
        }

        int matching_index = -1;

        if (AUTOMATICALLY_DOWNLOAD_MATCHING_PAPER_NAME) {
            // if a paper matches the query (almost) exactly, then download it without showing
            // a paper list to the user
            for (int i = 0; i < paper_titles.size(); i++) {
                if (does_paper_name_match_query(paper_name, paper_titles[i].toStdWString())) {
                    matching_index = i;
                    break;
                }
            }
        }

        if (matching_index > -1) {
            auto download_reply = download_paper_with_url(paper_urls[matching_index].toStdWString(), false, download_finish_action);
            QString sioyek_paper_name = QString::fromStdWString(paper_name);
            QString sioyek_actual_paper_name = paper_titles[matching_index];
            download_reply->setProperty("sioyek_paper_name", sioyek_paper_name);
            download_reply->setProperty("sioyek_actual_paper_name", sioyek_actual_paper_name);

            QObject::connect(download_reply, &QNetworkReply::finished, [this, download_reply, sioyek_paper_name, sioyek_actual_paper_name, download_finish_action, fn=std::move(fn)]() {
                int status_code = download_reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
                if (status_code == 200) {
                    fn(download_reply);
                }
                else if (status_code == 301) { // handle redirects
                    QString redirect_url = download_reply->header(QNetworkRequest::LocationHeader).toString();

                    auto redirect_download_reply = download_paper_with_url(redirect_url.toStdWString(), false, download_finish_action);
                    redirect_download_reply->setProperty("sioyek_paper_name", sioyek_paper_name);
                    redirect_download_reply->setProperty("sioyek_actual_paper_name", sioyek_actual_paper_name);
                    QObject::connect(redirect_download_reply, &QNetworkReply::finished, [this, redirect_download_reply, fn=std::move(fn)]() {
                        fn(redirect_download_reply);
                        });

                }
                });
            download_reply->setParent(parent); // fn should not be called if parent is deleted
        }
        else {
            // could not find the paper
        }

        });

    return reply;
}

void SioyekNetworkManager::download_unsynced_files(QObject* parent, DatabaseManager* db_manager) {

    QNetworkReply* reply = get_user_file_hash_set_reply();
    //authorize_request(&req);
    //req.setUrl(QUrl(QString::fromStdWString(SIOYEK_USER_FILE_HASH_SET_URL)));
    //QNetworkReply* reply = network_manager.get(req);
    //reply->setProperty("sioyek_handled", true);
    reply->setParent(parent);
    QObject::connect(reply, &QNetworkReply::finished, [this, parent, db_manager, reply]() {
        auto json_reply_ = get_network_json_reply(reply);
        if (json_reply_.has_value()) {
            QJsonObject json_object = json_reply_->object();
            //bool DatabaseManager::get_prev_path_hash_pairs(std::vector<std::pair<std::wstring, std::wstring>>& out_pairs) {

            std::vector<std::pair<std::wstring, std::wstring>> path_hash_pairs;
            db_manager->get_prev_path_hash_pairs(path_hash_pairs);
            std::vector<QString> hashes;
            std::vector<QString> server_hashes;
            for (auto [_, hash] : path_hash_pairs) {
                hashes.push_back(QString::fromStdWString(hash));
            }
            for (auto server_hash : json_object["results"].toArray()) {
                server_hashes.push_back(server_hash.toString());
            }
            std::sort(hashes.begin(), hashes.end());
            std::sort(server_hashes.begin(), server_hashes.end());
            std::vector<QString> new_hashes;

            std::set_difference(
                server_hashes.begin(), server_hashes.end(),
                hashes.begin(), hashes.end(),
                std::inserter(new_hashes, new_hashes.begin()));
            qDebug() << "should download these hashes: length = " << new_hashes.size();

            for (auto hash : new_hashes) {
                download_file_with_hash(parent, hash, [](QString) {

                    });
                //qDebug() << hash;
            }

            //qDebug() << json_object["results"];
        }
        //auto data = reply->readAll();
        //if (data.size() > 0) {
        //    auto json_resp = QJsonDocument::fromJson(data);
        //    if (!json_resp.isNull() && !json_resp.object().find("detail").value().isNull()) {
        //        QString error_msg = json_resp.object()["detail"].toString();
        //        show_error_message(error_msg.toStdWString());
        });

}

QNetworkReply* SioyekNetworkManager::download_paper_with_url(std::wstring paper_url_, bool use_archive_url, PaperDownloadFinishedAction action) {
    QString paper_url;
    if (use_archive_url) {
        paper_url = get_direct_pdf_url_from_archive_url(QString::fromStdWString(paper_url_));
    }
    else {
        paper_url = get_original_url_from_archive_url(QString::fromStdWString(paper_url_));
    }

    //paper_url = paper_url.right(paper_url.size() - paper_url.lastIndexOf("http"));
    QNetworkRequest req;
    req.setUrl(paper_url);

    if (use_archive_url) {
        std::string user_agent_string = get_user_agent_string();
        req.setRawHeader("User-Agent", user_agent_string.c_str());
    }

    auto res = network_manager.get(req);
    res->setProperty("sioyek_archive_url", QString::fromStdWString(paper_url_));
    res->setProperty("sioyek_finish_action", get_paper_download_finish_action_string(action));
    //res->setParent(&network_manager);
    //res->setParent(network_manager);
    return res;
}

bool SioyekNetworkManager::is_checksum_available_on_server(const std::string& checksum) {
    return SERVER_HASHES.find(checksum) != SERVER_HASHES.end();
}

bool SioyekNetworkManager::should_sync_location(){
    return last_document_location_upload_time.msecsTo(QDateTime::currentDateTime()) > 1000 * 60;
}

void SioyekNetworkManager::sync_file_location(QString hash, QString document_title, QString timestamp, float offset_y) {
    last_document_location_upload_time = QDateTime::currentDateTime();

    QNetworkRequest req;
    req.setUrl(QUrl(QString::fromStdWString(SIOYEK_SYNC_OPENED_BOOK_URL)));
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QJsonObject obj;
    obj["file_checksum"] = hash;
    obj["document_name"] = document_title;
    obj["offset_y"] = offset_y;
    obj["last_access_time"] = timestamp;

    QJsonDocument json_doc(obj);


    authorize_request(&req);

    QNetworkReply* reply = network_manager.post(req, json_doc.toJson());

    reply->setProperty("sioyek_handled", true);

    QObject::connect(reply, &QNetworkReply::finished, [this, reply]() {
        if (handle_network_reply_if_error(reply, false)) {
            qDebug() << reply->readAll();
        }
        });
}

QNetworkReply* SioyekNetworkManager::get_opened_book_data_from_checksum(QObject* parent, QString checksum, std::function<void(QJsonObject)> fn) {
    QUrl url(QString::fromStdWString(SIOYEK_GET_OPENED_BOOK_DATA_URL));
    QUrlQuery params;
    params.addQueryItem("file_checksum", checksum);
    url.setQuery(params);

    QNetworkRequest req;
    req.setUrl(url);
    authorize_request(&req);
    auto reply = network_manager.get(req);

    QObject::connect(reply, &QNetworkReply::finished, [this, reply, fn=std::move(fn)]() {
        if (handle_network_reply_if_error(reply, false)) {

            auto raw_data = reply->readAll();
            QJsonDocument json_doc = QJsonDocument::fromJson(raw_data);
            if (!json_doc.isNull()) {
                fn(json_doc.object());
            }
        }
        });
    // this is usually the widget that issued the request
    // otherwise fn might be called after widget is destroyed, which might
    // crash if fn does something with the widget
    reply->setParent(parent);
    return reply;

}

void SioyekNetworkManager::set_last_server_location(std::string checksum, float offset_y) {
    last_server_location[checksum] = offset_y;
}

void SioyekNetworkManager::download_opened_files_info(QObject* parent, std::function<void(QJsonObject)> fn) {

    //std::vector<std::string> local_checksums;
    //parent->db_manager->get_all_local_checksums(local_checksums);

    //QStringList checksums;
    //for (auto& checksum : local_checksums) {

    //    checksums.append(QString::fromStdString(checksum));
    //}

    QNetworkRequest req;
    req.setUrl(QUrl(QString::fromStdWString(SIOYEK_GET_OPENED_BOOKS_DATA_URL)));
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    //QJsonObject obj;
    //obj["file_checksums"] = QJsonArray::fromStringList(checksums);

    //QJsonDocument json_doc(obj);


    authorize_request(&req);

    QNetworkReply* reply = network_manager.get(req);

    if (parent) reply->setParent(parent);

    QObject::connect(reply, &QNetworkReply::finished, [this, reply, fn=std::move(fn)]() {
        auto data = reply->readAll();
        if (data.size() > 0) {
            QJsonDocument doc = QJsonDocument::fromJson(data);
            if (!doc.isNull()) {
                QJsonObject root = doc.object();
                if (root["status"].toString() != "OK") return;
                QJsonArray results = root["result"].toArray();

                server_opened_files.clear();

                for (int i = 0; i < results.size(); i++) {
                    QJsonObject result_object = results.at(i).toObject();
                    OpenedBookInfo opened_file_info;
                    opened_file_info.checksum = result_object["file_checksum"].toString().toStdString();
                    opened_file_info.document_title = result_object["document_name"].toString();
                    opened_file_info.file_name = result_object["file_name"].toString();
                    opened_file_info.last_access_time = QDateTime::fromString(result_object["last_access_time"].toString(), Qt::ISODate);
                    opened_file_info.offset_y = result_object["offset_y"].toDouble();
                    server_opened_files[opened_file_info.checksum] = opened_file_info;
                }

                fn(doc.object());
            }
        }
        });

}

 std::vector<OpenedBookInfo> SioyekNetworkManager::get_excluded_opened_files(std::vector<std::string>& excluded_checksums) {
    //std::unordered_map<QString, QString>  something;
     std::vector<std::string> server_checksums;
     for (auto& [checksum, _] : server_opened_files) {
         server_checksums.push_back(checksum);
     }

     std::sort(server_checksums.begin(), server_checksums.end());
     std::sort(excluded_checksums.begin(), excluded_checksums.end());

     std::vector<std::string> server_only_checksums;

    std::set_difference(
        server_checksums.begin(), server_checksums.end(),
        excluded_checksums.begin(), excluded_checksums.end(),
        std::back_inserter(server_only_checksums)
    );

    std::vector<OpenedBookInfo> server_only_files;
    for (auto checksum : server_only_checksums) {
        server_only_files.push_back(server_opened_files[checksum]);
    }

     return server_only_files;
}

void SioyekNetworkManager::handle_one_time_network_operations() {
    if (status == ServerStatus::LoggedIn) {
        if (!one_time_network_operations_performed) {
            update_user_files_hash_set();
            one_time_network_operations_performed = true;
            download_opened_files_info(nullptr, [&](QJsonObject obj) {
                });
        }
    }

}

void SioyekNetworkManager::upload_annot(
    QObject* parent,
    const QString& checksum,
    const Annotation& annot,
    std::function<void()> on_success,
    std::function<void()> on_fail) {

    QNetworkRequest req;
    req.setUrl(QUrl(QString::fromStdWString(get_url_for_annot_upload(&annot))));
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QJsonObject obj = annot.to_json(checksum.toStdString());
    obj["file_checksum"] = checksum;

    QJsonDocument json_doc(obj);

    authorize_request(&req);

    QNetworkReply* reply = network_manager.post(req, json_doc.toJson());
    QObject::connect(reply, &QNetworkReply::finished, [this, reply, on_success=std::move(on_success), on_fail=std::move(on_fail)]() {
        int status_code = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        if (status_code == 200) {
            on_success();
        }
        else {
            on_fail();
        }
        });
}

void SioyekNetworkManager::delete_annot(QObject* parent, const QString& file_checksum, const QString& annot_type, const QString& uuid, std::function<void()> on_success, std::function<void()> on_fail) {

    QNetworkRequest req;
    req.setUrl(QUrl(QString::fromStdWString(SIOYEK_DELETE_ANNOT_URL)));
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QJsonObject obj;
    obj["uuid"] = uuid;
    obj["file_checksum"] = file_checksum;
    obj["annot_type"] = annot_type;

    QJsonDocument json_doc(obj);

    authorize_request(&req);

    QNetworkReply* reply = network_manager.post(req, json_doc.toJson());
    reply->setParent(parent);
    QObject::connect(reply, &QNetworkReply::finished, [this, reply, on_fail=std::move(on_fail), on_success=std::move(on_success)]() {
        int status_code = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        if (status_code != 200) {
            on_fail();
        }
        else {
            on_success();
        }
        });
}

void SioyekNetworkManager::get_document_annotations(QObject* parent, const QString& document_checksum, std::function<void(std::vector<Highlight>&&, std::vector<BookMark>&&, std::vector<Portal>&&, std::optional<QDateTime>)> fn){
    QUrl url(QString::fromStdWString(SIOYEK_GET_DOCUMENT_ANNOTATIONS_URL));
    QUrlQuery params;
    params.addQueryItem("file_checksum", document_checksum);
    url.setQuery(params);

    QNetworkRequest req;
    req.setUrl(url);
    authorize_request(&req);
    auto reply = network_manager.get(req);
    reply->setParent(parent);
    QObject::connect(reply, &QNetworkReply::finished, [this, reply, fn=std::move(fn)]() {

        int status_code = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        if (status_code == 200) {
            auto data = reply->readAll();
            QJsonObject result_object = QJsonDocument::fromJson(data).object();

            if (result_object["status"].toString() != "OK") return;

            std::optional<QDateTime> last_access_time = {};
            if (!result_object["last_access_time"].isNull()) {
                last_access_time = QDateTime::fromString(result_object["last_access_time"].toString(), Qt::DateFormat::ISODate);
                last_access_time->setTimeSpec(Qt::UTC);
            }

            QJsonArray json_highlights = result_object["highlights"].toArray();
            QJsonArray json_bookmarks = result_object["bookmarks"].toArray();
            QJsonArray json_portals = result_object["portals"].toArray();

            std::vector<Highlight> res_highlights;
            std::vector<BookMark> res_bookmarks;
            std::vector<Portal> res_portals;

            for (int i = 0; i < json_highlights.size(); i++) {
                QJsonObject json_highlight = json_highlights[i].toObject();

                Highlight highlight = Highlight::from_json(json_highlight);
                res_highlights.push_back(highlight);
            }
            for (int i = 0; i < json_bookmarks.size(); i++) {
                QJsonObject json_bookmark = json_bookmarks[i].toObject();

                BookMark bookmark = BookMark::from_json(json_bookmark);
                res_bookmarks.push_back(bookmark);
            }

            for (int i = 0; i < json_portals.size(); i++) {
                QJsonObject json_portal = json_portals[i].toObject();

                Portal portal = Portal::from_json(json_portal);
                res_portals.push_back(portal);
            }

            fn(std::move(res_highlights), std::move(res_bookmarks), std::move(res_portals), last_access_time);

        }

        });
}

void SioyekNetworkManager::perform_unsynced_inserts_and_deletes(QObject* parent, const QString& checksum, std::function<void()> on_done) {
    std::vector<std::pair<std::wstring, std::wstring>> unsynced_deletes;

    //std::vector<std::string> unsynced_highlight_insert_uuids;
    //std::unordered_set<std::string> unsynced_highlight_uuids_set;
    //std::vector<std::string> unsynced_bookmark_insert_uuids;
    //std::unordered_set<std::string> unsynced_bookmark_uuids_set;

    //std::vector<std::string> unsynced_portal_insert_uuids;
    //std::unordered_set<std::string> unsynced_portal_uuids_set;

    //std::vector<Highlight> unsycned_highlights;
    //std::vector<BookMark> unsynced_bookmarks;
    //std::vector<BookMark> unsynced_portals;


    //auto get_unsynced_uuids = [&](const std::string& table_name) -> std::pair<std::vector<std::string>, std::unordered_set<std::string>> {
    //    std::vector<std::string> res_vector;
    //    std::unordered_set<std::string> res_set;

    //    parent->db_manager->get_document_unsynced_table_uuids(table_name, checksum.toStdString(), res_vector);
    //    for (auto& uuid : res_vector) {
    //        res_set.insert(uuid);
    //    }
    //    return std::make_pair(res_vector, res_set);

    //    };

    //auto [unsynced_highlight_insert_uuids, unsynced_highlight_uuids_set] = get_unsynced_uuids("highlights");
    //auto [unsynced_bookmark_insert_uuids, unsynced_bookmark_uuids_set] = get_unsynced_uuids("bookmarks");
    //auto [unsynced_portal_insert_uuids, unsynced_portal_uuids_set] = get_unsynced_uuids("portals");

    //for (auto& hl : parent->doc()->get_highlights()) {
    //    if (unsynced_highlight_uuids_set.find(hl.uuid) != unsynced_highlight_uuids_set.end()) {
    //        unsycned_highlights.push_back(hl);
    //    }
    //}

    //for (auto& bm : parent->doc()->get_bookmarks()) {
    //    if (unsynced_bookmark_uuids_set.find(bm.uuid) != unsynced_bookmark_uuids_set.end()) {
    //        unsynced_bookmarks.push_back(bm);
    //    }
    //}

    db_manager->get_all_unsynced_deletions(checksum.toStdString(), unsynced_deletes);

    Document* doc = document_manager->get_document_with_checksum(checksum.toStdString());
    if (doc == nullptr) return;

    std::vector<Highlight> unsynced_highlights = doc->get_unsynced_annots<Highlight>();
    std::vector<BookMark> unsynced_bookmarks = doc->get_unsynced_annots<BookMark>();
    std::vector<Portal> unsynced_portals = doc->get_unsynced_annots<Portal>();
    std::vector<std::string> unsynced_highlight_uuids = get_uuids(unsynced_highlights);
    std::vector<std::string> unsynced_bookmark_uuids = get_uuids(unsynced_bookmarks);
    std::vector<std::string> unsynced_portal_uuids = get_uuids(unsynced_portals);

    if (unsynced_deletes.size() == 0 && unsynced_highlights.size() == 0 && unsynced_bookmarks.size() == 0 && unsynced_portals.size() == 0) {
        on_done();
    }
    else {
        // 
        QNetworkRequest req;
        req.setUrl(QUrl(QString::fromStdWString(SIOYEK_SYNC_DELETES_URL)));
        req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

        QJsonArray deletes_array;
        for (auto& [type, uuid] : unsynced_deletes) {
            QJsonObject obj;
            obj["type"] = QString::fromStdWString(type);
            obj["uuid"] = QString::fromStdWString(uuid);
            deletes_array.push_back(obj);
        }

        QJsonArray highlights_array;
        QJsonArray bookmarks_array;
        QJsonArray portals_array;

        for (auto& hl : unsynced_highlights) {
            highlights_array.append(hl.to_json(checksum.toStdString()));
        }

        for (auto& bm : unsynced_bookmarks) {
            bookmarks_array.append(bm.to_json(checksum.toStdString()));
        }

        for (auto& portal : unsynced_portals) {
            portals_array.append(portal.to_json(checksum.toStdString()));
        }

        QJsonObject obj;
        obj["unsynced_deletes"] = deletes_array;
        obj["highlights"] = highlights_array;
        obj["bookmarks"] = bookmarks_array;
        obj["portals"] = portals_array;
        obj["file_checksum"] = checksum;

        QJsonDocument json_doc(obj);


        authorize_request(&req);

        QNetworkReply* reply = network_manager.post(req, json_doc.toJson());
        QObject::connect(reply, &QNetworkReply::finished, [
                this,
                checksum,
                unsynced_highlight_uuids = std::move(unsynced_highlight_uuids),
                unsynced_bookmark_uuids = std::move(unsynced_bookmark_uuids),
                unsynced_portal_uuids = std::move(unsynced_portal_uuids),
                parent,
                reply, on_done = std::move(on_done)
        ]() {
            qDebug() << reply->readAll();
            int status_code = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
            if (status_code == 200) {
                db_manager->clear_unsynced_deletions(checksum.toStdString());
                Document* doc = document_manager->get_document_with_checksum(checksum.toStdString());
                if (doc == nullptr) return;

                doc->set_annots_to_synced<Highlight>(unsynced_highlight_uuids);
                doc->set_annots_to_synced<BookMark>(unsynced_bookmark_uuids);
                doc->set_annots_to_synced<Portal>(unsynced_portal_uuids);
                //parent->db_manager->set_annot_uuids_to_synced("highlights", unsynced_highlight_uuids);
                //parent->db_manager->set_annot_uuids_to_synced("bookmarks", unsynced_bookmark_uuids);
                //parent->db_manager->
                //parent->db_manager->clear_unsynced_additions(checksum.toStdString());
            }
            on_done();
            });
    }
}

const std::wstring& SioyekNetworkManager::get_url_for_annot_upload(const Annotation* annot) {
    if (dynamic_cast<const Highlight*>(annot)) {
        return SIOYEK_ADD_HIGHLIGHT_URL;
    }
    if (dynamic_cast<const BookMark*>(annot)) {
        return SIOYEK_ADD_BOOKMARK_URL;
    }
    if (dynamic_cast<const Portal*>(annot)) {
        return SIOYEK_ADD_PORTAL_URL;
    }

    return L"";
}

void SioyekNetworkManager::delete_file_with_checksum(const QString& checksum) {

    if (SERVER_HASHES.find(checksum.toStdString()) != SERVER_HASHES.end()) {
        SERVER_HASHES.erase(checksum.toStdString());
    }

    QNetworkRequest req;
    req.setUrl(QUrl(QString::fromStdWString(SIOYEK_DELETE_FILE_URL)));
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QJsonObject obj;
    obj["file_checksum"] = checksum;

    QJsonDocument json_doc(obj);

    authorize_request(&req);

    QNetworkReply* reply = network_manager.post(req, json_doc.toJson());
}

void SioyekNetworkManager::tts(QObject* parent, const std::wstring& text, const std::string& document_checksum, int page, float rate, std::function<void(QString, std::vector<float>)> on_done) {
    QString text_checksum = QString::fromStdString(compute_md5_from_data(QString::fromStdWString(text).toUtf8()));
    QString file_path = QString::fromStdWString(cached_tts_path.slash( text_checksum.toStdWString() + L".mp3").get_path());
    QString timestamps_file_path = QString::fromStdWString(cached_tts_path.slash(text_checksum.toStdWString() + L".json").get_path());
    if (rate != 1) {
        file_path  =  QString::fromStdWString(cached_tts_path.slash( text_checksum.toStdWString() + L"_" + QString::number(rate).toStdWString() + L".mp3").get_path());
        timestamps_file_path =  QString::fromStdWString(cached_tts_path.slash(text_checksum.toStdWString() + L"_" + QString::number(rate).toStdWString() + L".json").get_path());
    }
    QFileInfo timestamps_file_info(timestamps_file_path);
    QFileInfo audio_file_info(file_path);

    if (timestamps_file_info.exists() && audio_file_info.exists()){
        QFile timestamps_file(timestamps_file_path);
        if (timestamps_file.open(QIODeviceBase::ReadOnly)) {
            QJsonDocument json_doc = QJsonDocument::fromJson(timestamps_file.readAll());
            timestamps_file.close();

            QJsonArray items = json_doc.array();
            std::vector<float> timestamps;
            for (int i = 0; i < items.size(); i++) {
                timestamps.push_back(items[i].toDouble());
            }
            on_done(file_path, timestamps);
        }

    }
    else {
        QNetworkRequest req;
        req.setUrl(QUrl(QString::fromStdWString(SIOYEK_TTS_URL)));
        req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

        QJsonObject obj;
        obj["text"] = QString::fromStdWString(text);
        obj["text_checksum"] = text_checksum;
        obj["document_checksum"] = QString::fromStdString(document_checksum);
        obj["page"] = page;
        obj["rate"] = rate;

        QJsonDocument json_doc(obj);
        authorize_request(&req);

        QNetworkReply* reply = network_manager.post(req, json_doc.toJson());
        reply->setParent(parent);
        reply->setProperty("sioyek_network_status_string", "Creating audio");
        QObject::connect(reply, &QNetworkReply::finished, [reply, file_path, timestamps_file_path, on_done = std::move(on_done)]() {
            reply->deleteLater();

            int status_code = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
            if (status_code == 200) {
                QJsonDocument json_doc = QJsonDocument::fromJson(reply->rawHeader("timestamps"));
                QJsonArray items = json_doc.array();
                std::vector<float> timestamps;
                for (int i = 0; i < items.size(); i++) {
                    timestamps.push_back(items[i].toDouble());
                }

                QByteArray audio_data = reply->readAll();

                if (!cached_tts_path.dir_exists()) {
                    QDir dir = QDir(QString::fromStdWString(standard_data_path.get_path()));
                    dir.mkdir("tts");
                }

                QFile file(file_path);
                if (file.open(QIODeviceBase::WriteOnly)) {
                    file.write(audio_data);
                }
                file.close();

                QFile timestamps_file(timestamps_file_path);
                if (timestamps_file.open(QIODeviceBase::WriteOnly)) {
                    int written = timestamps_file.write(json_doc.toJson());
                    qDebug() << "write " << written;
                }
                timestamps_file.close();
                on_done(file_path, timestamps);
            }
            else {
                //qDebug() << "something bad happened";
                //QFile access_token_file(QString::fromStdWString(sioyek_access_token_path.get_path()));
            }
            });

    }

}

void SioyekNetworkManager::debug(QObject* parent, std::function<void()> on_done) {
    QNetworkRequest req;
    req.setUrl(QUrl(QString::fromStdWString(SIOYEK_DEBUG_URL)));
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QJsonObject obj;
    obj["secs"] = 4;

    QJsonDocument json_doc(obj);
    authorize_request(&req);

    QNetworkReply* reply = network_manager.post(req, json_doc.toJson());
    reply->setParent(parent);
    reply->setProperty("sioyek_network_status_string", "debug netowkr erquest");
    QObject::connect(reply, &QNetworkReply::finished, [on_done=std::move(on_done)]() {
        on_done();
        });
}

void SioyekNetworkManager::semantic_search(QObject* parent, const QString& query, const std::wstring& index, std::function<void(QJsonObject response)> on_done) {
    QString index_qstring = QString::fromStdWString(index); // todo: performance: we should prevent this as much as possible
    std::string content_checksum = compute_md5_from_data(index_qstring.toUtf8());

    QNetworkRequest req;
    req.setUrl(QUrl(QString::fromStdWString(SIOYEK_SEMANTIC_SEARCH_URL)));
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QJsonObject obj;
    obj["content_checksum"] = QString::fromStdString(content_checksum);
    obj["query"] = query;

    QJsonDocument json_doc(obj);
    authorize_request(&req);

    QNetworkReply* reply = network_manager.post(req, json_doc.toJson());
    reply->setParent(parent);
    reply->setProperty("sioyek_network_status_string", "performing semantic search");
    QObject::connect(reply, &QNetworkReply::finished, [reply, on_done=std::move(on_done)]() {
        int status_code = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        if (status_code == 200) {
            QByteArray raw_response = reply->readAll();
            QJsonDocument json_document = QJsonDocument::fromJson(raw_response);
            on_done(json_document.object());
        }
        //on_done();
        });

}

void SioyekNetworkManager::upload_document_index(QObject* parent, const std::wstring& document_content, std::function<void(QJsonObject)> on_done) {
    QString index_qstring = QString::fromStdWString(document_content);
    std::string content_checksum = compute_md5_from_data(index_qstring.toUtf8());

    QNetworkRequest req;
    req.setUrl(QUrl(QString::fromStdWString(SIOYEK_UPLOAD_INDEX_URL)));
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QJsonObject obj;
    obj["content_checksum"] = QString::fromStdString(content_checksum);
    obj["text"] = index_qstring;

    QJsonDocument json_doc(obj);
    authorize_request(&req);

    QNetworkReply* reply = network_manager.post(req, json_doc.toJson());
    reply->setParent(parent);
    reply->setProperty("sioyek_network_status_string", "creating semantic index");
    QObject::connect(reply, &QNetworkReply::finished, [reply, on_done=std::move(on_done)]() {
        int status_code = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        if (status_code == 200) {
            QByteArray raw_response = reply->readAll();
            QJsonDocument json_document = QJsonDocument::fromJson(raw_response);
            on_done(json_document.object());
        }
        //on_done();
        });
}
