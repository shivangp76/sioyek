#include "network_manager.h"

#include <qurlquery.h>
#include <qnetworkreply.h>
#include <qjsondocument.h>
#include <qfile.h>
#include <qdir.h>
#include <qhttpmultipart.h>
#include <qtimer.h>
#include <qapplication.h>
#include <QtCore/qbuffer.h>

#include "utils.h"
#include "path.h"
#include "checksum.h"
#include "database.h"
#include "document.h"
#include "background_tasks.h"

extern std::string APPLICATION_VERSION;
extern Path sioyek_access_token_path;
extern Path cached_tts_path;
extern Path standard_data_path;
extern std::wstring PAPERS_FOLDER_PATH;
extern bool AUTOMATICALLY_DOWNLOAD_MATCHING_PAPER_NAME;
extern std::wstring EXTRACT_TABLE_PROMPT;
extern Path sioyek_json_data_path;
extern std::wstring SIOYEK_HOST;

extern Path downloaded_papers_path;

QNetworkAccessManager* SioyekNetworkManager::get_network_manager() {
    if (network_manager_) {
        return network_manager_;
    }
    network_manager_ = new QNetworkAccessManager();
    QObject::connect(network_manager_, &QNetworkAccessManager::finished, [](QNetworkReply* reply) {
        reply->deleteLater();
        });

#ifdef SIOYEK_DEVELOPER
    QObject::connect(network_manager_, &QNetworkAccessManager::sslErrors, [](QNetworkReply* reply, const QList<QSslError>& errors) {
        reply->ignoreSslErrors();
        });
#endif

    return network_manager_;
}

SioyekNetworkManager::SioyekNetworkManager(DatabaseManager* db_manager_, BackgroundTaskManager* task_manager, DocumentManager* doc_manager, QObject* parent) :
   db_manager(db_manager_), background_task_manager(task_manager) , document_manager(doc_manager) {
    last_document_location_upload_time = QDateTime::currentDateTime();
}

void SioyekNetworkManager::login(std::wstring email, std::wstring password) {
    QNetworkRequest req;
    req.setUrl(QUrl(QString::fromStdWString(SIOYEK_HOST + SIOYEK_TOKEN_URL_)));
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
    QUrlQuery params;
    params.addQueryItem("username", QString::fromStdWString(email));
    params.addQueryItem("password", QString::fromStdWString(password));
    QNetworkReply* reply = get_network_manager()->post(req, params.query().toUtf8());

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
            QUrl url(QString::fromStdWString(SIOYEK_HOST + SIOYEK_ECHO_URL_));

            QNetworkRequest req;
            authorize_request(&req);
            req.setUrl(url);
            auto reply = get_network_manager()->get(req);
            reply->setProperty("sioyek_handled", true);
            //
            status = ServerStatus::LoggingIn;
            QObject::connect(reply, &QNetworkReply::finished, [this, reply]() {
                int status_code = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
                QString content = reply->readAll();
                if (status_code == 401) {
                    ACCESS_TOKEN = "";
                    persist_access_token(ACCESS_TOKEN);
                    show_error_message(L"Access token was expired, please login again");
                }
                else if (status_code == 0) {
                    status = ServerStatus::ServerOffline;
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
    QUrl url(QString::fromStdWString(SIOYEK_HOST + SIOYEK_DOWNLOAD_FILE_WITH_HASH_PATH_));
    QUrlQuery query;
    query.addQueryItem("hash", hash);
    url.setQuery(query);


    QNetworkRequest req;
    authorize_request(&req);
    req.setUrl(url);
    QNetworkReply* reply = get_network_manager()->get(req);
    reply->setProperty("sioyek_handled", true);
    reply->setParent(parent);
    QObject::connect(reply, &QNetworkReply::finished, [this, reply, hash, fn=std::move(fn)]() {

        QString filename = reply->header(QNetworkRequest::ContentDispositionHeader).toString().mid(22);
        filename = filename.left(filename.size() - 1);
        QDir papers_dir;
        if (PAPERS_FOLDER_PATH.size() > 0){
            papers_dir = QDir(QString::fromStdWString(PAPERS_FOLDER_PATH));
        }
        else{
            papers_dir = QDir(QString::fromStdWString(downloaded_papers_path.get_path()));
        }
        if (!papers_dir.exists()){
            QDir parent = papers_dir;
            parent.cdUp();
            parent.mkdir(papers_dir.dirName());
        }

        QString download_path = papers_dir.absoluteFilePath(filename);
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
                    else{
                        qDebug() << "could not open the file";
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
    req.setUrl(QUrl(QString::fromStdWString(SIOYEK_HOST + SIOYEK_USER_FILE_HASH_SET_URL_)));
    QNetworkReply* reply = get_network_manager()->get(req);
    reply->setProperty("sioyek_handled", true);
    return reply;
}

void SioyekNetworkManager::ocr_file(QObject * parent, QString path, std::function<void(QNetworkReply* download_reply)> fn){
    QFile* file = new QFile(path);
    if (file->open(QIODevice::ReadOnly)) {
        QHttpMultiPart* parts = new QHttpMultiPart(QHttpMultiPart::FormDataType);

        QJsonDocument json_doc;
        QJsonObject root_object;
        root_object["sioyek_version"] = QString::fromStdString(APPLICATION_VERSION);
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
        req.setUrl(QUrl(QString::fromStdWString(SIOYEK_HOST + SIOYEK_OCR_URL_)));
        //qDebug() << "boundary is:" << parts->boundary();
        //qDebug() << "file size  is:" << file->size();

        file->setParent(parts);
        // DO NOT use the Qt's default boundary, it does not work
        parts->setBoundary(create_random_string().toUtf8());
        req.setHeader(QNetworkRequest::KnownHeaders::ContentTypeHeader, "multipart/form-data; boundary=" + parts->boundary());

        QNetworkReply* reply = get_network_manager()->post(req, parts);
        reply->setProperty("sioyek_handled", true);
        reply->setParent(parent);

        QObject::connect(reply, &QNetworkReply::finished, [this, reply, fn=std::move(fn)]() {

            if (handle_network_reply_if_error(reply, true)) {
                fn(reply);
            }
            });

        parts->setParent(reply);
    }
}

void SioyekNetworkManager::upload_file(
    QObject* parent,
    QString path,
    QString hash,
    std::function<void()> done_fn,
    std::optional<std::function<void(int, int)>> progress_fn
) {
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
        req.setUrl(QUrl(QString::fromStdWString(SIOYEK_HOST + SIOYEK_UPLOAD_URL_)));
        //qDebug() << "boundary is:" << parts->boundary();
        //qDebug() << "file size  is:" << file->size();

        file->setParent(parts);
        // DO NOT use the Qt's default boundary, it does not work
        parts->setBoundary(create_random_string().toUtf8());
        req.setHeader(QNetworkRequest::KnownHeaders::ContentTypeHeader, "multipart/form-data; boundary=" + parts->boundary());

        QNetworkReply* reply = get_network_manager()->post(req, parts);
        reply->setProperty("sioyek_handled", true);
        reply->setParent(parent);

        QObject::connect(reply, &QNetworkReply::uploadProgress, [this, reply, progress_fn=std::move(progress_fn)](qint64 bytesSent, qint64 bytesTotal) {
            if (progress_fn) {
                progress_fn.value()(bytesSent, bytesTotal);
            }
            });

        QObject::connect(reply, &QNetworkReply::finished, [this, reply, done_fn=std::move(done_fn)]() {

            if (handle_network_reply_if_error(reply, true)) {
                update_user_files_hash_set();
                auto json_resp = QJsonDocument::fromJson(reply->readAll());
                if (json_resp["status"] != "OK" && json_resp["type"] == "incorrect_file_hash") {
                    //todo: update_current_document_checksum()

                    show_error_message(L"the file hash was incorrect");
                }
                else {
                    done_fn();
                }
            }
            });

        parts->setParent(reply);
    }
}

void SioyekNetworkManager::update_checksum(QObject* parent, QString path, QString old_checksum, QString new_checksum, std::function<void()> fn) {
    QFile* file = new QFile(path);
    if (file->open(QIODevice::ReadOnly)) {
        QHttpMultiPart* parts = new QHttpMultiPart(QHttpMultiPart::FormDataType);
        QByteArray data = file->readAll();

        QJsonDocument json_doc;
        QJsonObject root_object;
        root_object["sioyek_version"] = QString::fromStdString(APPLICATION_VERSION);
        root_object["old_checksum"] = old_checksum;
        root_object["new_checksum"] = new_checksum;
        json_doc.setObject(root_object);


        QHttpPart data_part;
        data_part.setHeader(QNetworkRequest::ContentDispositionHeader, QVariant("form-data; name=\"sioyek_data\""));
        data_part.setBody(json_doc.toJson(QJsonDocument::JsonFormat::Compact));
        parts->append(data_part);

        QHttpPart file_part;
        file_part.setHeader(QNetworkRequest::ContentDispositionHeader, "form-data; name=\"file\"; filename=\"" + file->fileName() + "\"");

        // ideally we should just be able to use setBodyDevice(file) which should use less memory, however,
        // this sometimes causes a crash when latex software puts the file in an invalid state so we just read
        // the entire file here
        file_part.setBody(data);

        parts->append(file_part);


        QNetworkRequest req;
        authorize_request(&req);
        req.setUrl(QUrl(QString::fromStdWString(SIOYEK_HOST + SIOYEK_UPDATE_CHECKSUM_URL_)));
        //qDebug() << "boundary is:" << parts->boundary();
        //qDebug() << "file size  is:" << file->size();

        file->setParent(parts);
        // DO NOT use the Qt's default boundary, it does not work
        parts->setBoundary(create_random_string().toUtf8());
        req.setHeader(QNetworkRequest::KnownHeaders::ContentTypeHeader, "multipart/form-data; boundary=" + parts->boundary());

        QNetworkReply* reply = get_network_manager()->post(req, parts);
        reply->setProperty("sioyek_handled", true);
        reply->setParent(parent);

        QObject::connect(reply, &QNetworkReply::finished, [this, reply, fn=std::move(fn)]() {
            reply->deleteLater();

            if (handle_network_reply_if_error(reply, true)) {
                fn();
                //update_user_files_hash_set();
                //auto json_resp = QJsonDocument::fromJson(reply->readAll());
                //if (json_resp["status"] != "OK" && json_resp["type"] == "incorrect_file_hash") {
                //    //todo: update_current_document_checksum()

                //    show_error_message(L"the file hash was incorrect");
                //}
                //else {
                //    fn();
                //}
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
                server_hashes_loaded = true;
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

QNetworkReply* SioyekNetworkManager::get_citers_with_name(
    QObject* parent, const std::wstring& name, std::function<void(QNetworkReply*)> fn) {

    QUrl url(QString::fromStdWString(SIOYEK_HOST + SIOYEK_PAPER_CITERS_URL_));
    QUrlQuery params;
    params.addQueryItem("paper_title", QString::fromStdWString(name));
    url.setQuery(params);

    QNetworkRequest req;
    std::string user_agent_string = get_user_agent_string();
    req.setRawHeader("User-Agent", user_agent_string.c_str());
    req.setUrl(url);
    authorize_request(&req);
    auto reply = get_network_manager()->get(req);

    reply->setProperty("sioyek_downloading", true);
    reply->setProperty("sioyek_network_message", "Retrieving citations ...");

    QObject::connect(reply, &QNetworkReply::finished, [this, reply, fn = std::move(fn)]() {
        if (handle_network_reply_if_error(reply, true)) {
            fn(reply);
        }
        });
    return reply;
}

QNetworkReply* SioyekNetworkManager::download_paper_with_name(
    QObject* parent,
    const std::wstring& name,
    QString full_bib_text,
    PaperDownloadFinishedAction action,
    std::function<void(QNetworkReply*)> begin_function,
    std::function<void(QNetworkReply*)> fn
    ) {
    std::wstring download_name = name;
    if (name.size() > 0 && name[0] == ':') {
        download_name = name.substr(1, name.size() - 1);
    }

    QUrl url(QString::fromStdWString(SIOYEK_HOST + SIOYEK_PAPER_URL_URL_));
    QUrlQuery params;
    params.addQueryItem("paper_title", QString::fromStdWString(download_name));
    params.addQueryItem("full_bib_text", full_bib_text);
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
    auto reply = get_network_manager()->get(req);
    reply->setProperty("sioyek_paper_name", QString::fromStdWString(name));
    reply->setProperty("sioyek_finish_action", get_paper_download_finish_action_string(action));
    reply->setProperty("sioyek_downloading", true);
    QObject::connect(reply, &QNetworkReply::finished, [this, reply, parent, fn=std::move(fn), begin_function=std::move(begin_function)]() {

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

        bool title_was_corrected = !json_doc["correct_title"].isNull();

        int matching_index = -1;

        if (AUTOMATICALLY_DOWNLOAD_MATCHING_PAPER_NAME) {
            // if a paper matches the query (almost) exactly, then download it without showing
            // a paper list to the user
            for (int i = 0; i < paper_titles.size(); i++) {
                if (title_was_corrected || does_paper_name_match_query(paper_name, paper_titles[i].toStdWString())) {
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
            download_reply->setProperty("sioyek_downloading", true);

            //qDebug() << "downlaod_reply: " << download_reply;
            //QObject::connect(download_reply, &QNetworkReply::downloadProgress, [](qint64 r, qint64 t) {
            //    qDebug() << r;
            //    });
            begin_function(download_reply);

            QObject::connect(download_reply, &QNetworkReply::finished, [this, download_reply, sioyek_paper_name, sioyek_actual_paper_name, download_finish_action, begin_function=std::move(begin_function), fn=std::move(fn)]() {
                int status_code = download_reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
                if (status_code == 200) {
                    fn(download_reply);
                }
                else if (status_code == 301) { // handle redirects
                    QString redirect_url = download_reply->header(QNetworkRequest::LocationHeader).toString();

                    auto redirect_download_reply = download_paper_with_url(redirect_url.toStdWString(), false, download_finish_action);
                    redirect_download_reply->setProperty("sioyek_paper_name", sioyek_paper_name);
                    redirect_download_reply->setProperty("sioyek_actual_paper_name", sioyek_actual_paper_name);
                    redirect_download_reply->setProperty("sioyek_downloading", true);
                    begin_function(redirect_download_reply);
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

    auto res = get_network_manager()->get(req);
    res->setProperty("sioyek_archive_url", QString::fromStdWString(paper_url_));
    res->setProperty("sioyek_finish_action", get_paper_download_finish_action_string(action));
    //qDebug() <<  "res:" << res;
    //QObject::connect(res, &QNetworkReply::downloadProgress, [](qint64 total, qint64 received) {
    //    qDebug() << "received " << received << " total " << total;
    //    });
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

QNetworkReply* SioyekNetworkManager::sync_file_location(QString hash, QString document_title, QString timestamp, float offset_y) {
    last_document_location_upload_time = QDateTime::currentDateTime();

    QNetworkRequest req;
    req.setUrl(QUrl(QString::fromStdWString(SIOYEK_HOST + SIOYEK_SYNC_OPENED_BOOK_URL_)));
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QJsonObject obj;
    obj["file_checksum"] = hash;
    obj["document_name"] = document_title;
    obj["offset_y"] = offset_y;
    obj["last_access_time"] = timestamp;

    QJsonDocument json_doc(obj);


    authorize_request(&req);

    QNetworkReply* reply = get_network_manager()->post(req, json_doc.toJson());

    reply->setProperty("sioyek_handled", true);

    QObject::connect(reply, &QNetworkReply::finished, [this, reply]() {
        if (handle_network_reply_if_error(reply, false)) {
            qDebug() << reply->readAll();
        }
        });
    return reply;
}

QNetworkReply* SioyekNetworkManager::get_opened_book_data_from_checksum(QObject* parent, QString checksum, std::function<void(QJsonObject)> fn) {
    QUrl url(QString::fromStdWString(SIOYEK_HOST + SIOYEK_GET_OPENED_BOOK_DATA_URL_));
    QUrlQuery params;
    params.addQueryItem("file_checksum", checksum);
    url.setQuery(params);

    QNetworkRequest req;
    req.setUrl(url);
    authorize_request(&req);
    auto reply = get_network_manager()->get(req);

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
    req.setUrl(QUrl(QString::fromStdWString(SIOYEK_HOST + SIOYEK_GET_OPENED_BOOKS_DATA_URL_)));
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    //QJsonObject obj;
    //obj["file_checksums"] = QJsonArray::fromStringList(checksums);

    //QJsonDocument json_doc(obj);


    authorize_request(&req);

    QNetworkReply* reply = get_network_manager()->get(req);

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
        server_opened_files[checksum].is_server_only = true;
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

    QNetworkReply* reply = get_network_manager()->post(req, json_doc.toJson());
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


void SioyekNetworkManager::perform_generic_llm_request(QObject* parent, const QString& system_prompt, const QString& user_prompt, const QPixmap* pixmap, std::function<void(QString)> on_done) {

    QNetworkRequest req;
    req.setUrl(QUrl(QString::fromStdWString(SIOYEK_HOST + SIOYEK_GENERIC_LLM_URL_)));
    authorize_request(&req);

    QHttpMultiPart* multipart = new QHttpMultiPart(QHttpMultiPart::FormDataType);

    if (pixmap) {
        QByteArray image_data;
        QBuffer image_buffer(&image_data);
        image_buffer.open(QIODevice::WriteOnly);
        pixmap->save(&image_buffer, "PNG");

        QHttpPart image_part;
        image_part.setHeader(QNetworkRequest::ContentTypeHeader, QVariant("image/png"));
        image_part.setHeader(QNetworkRequest::ContentDispositionHeader, QVariant("form-data; name=\"image\"; filename=\"image.png\""));
        image_part.setBody(image_data);
        multipart->append(image_part);
    }

    QJsonDocument json_doc;
    QJsonObject root_object;
    root_object["system_prompt"] = system_prompt;
    root_object["user_prompt"] = user_prompt;
    //root_object["prompt"] = prompt.value_or(QString::fromStdWString(EXTRACT_TABLE_PROMPT));
    json_doc.setObject(root_object);


    QHttpPart data_part;
    data_part.setHeader(QNetworkRequest::ContentDispositionHeader, QVariant("form-data; name=\"sioyek_data\""));
    data_part.setBody(json_doc.toJson(QJsonDocument::JsonFormat::Compact));
    multipart->append(data_part);


    QNetworkReply* reply = get_network_manager()->post(req, multipart);
    reply->setParent(parent);
    reply->setProperty("sioyek_network_status_string", "extracting data from image");

    QObject::connect(reply, &QNetworkReply::finished, [reply, on_done=std::move(on_done)]() {
        auto data = reply->readAll();
        QJsonObject result_object = QJsonDocument::fromJson(data).object();
        QString status = result_object["status"].toString();
        if (status == "OK") {
            on_done(result_object["result"].toString());
        }
        });


}

void SioyekNetworkManager::semantic_ask_with_image(
    QObject * parent,
    const std::wstring& document_content,
    const QPixmap& pixmap,
    std::function<void(QString)>&& on_chunk,
    std::function<void()>&& on_done
    ){
    QByteArray image_data;
    QBuffer image_buffer(&image_data);
    image_buffer.open(QIODevice::WriteOnly);
    pixmap.save(&image_buffer, "PNG");

    QNetworkRequest req;
    req.setUrl(QUrl(QString::fromStdWString(SIOYEK_HOST + SIOYEK_SEMANTIC_ASK_WITH_IMAGE_URL_)));
    authorize_request(&req);

    QHttpMultiPart* multipart = new QHttpMultiPart(QHttpMultiPart::FormDataType);

    QHttpPart image_part;
    image_part.setHeader(QNetworkRequest::ContentTypeHeader, QVariant("image/png"));
    image_part.setHeader(QNetworkRequest::ContentDispositionHeader, QVariant("form-data; name=\"image\"; filename=\"image.png\""));
    image_part.setBody(image_data);

    QJsonDocument json_doc;
    QJsonObject root_object;
    root_object["document_content"] = QString::fromStdWString(document_content);
    json_doc.setObject(root_object);


    QHttpPart data_part;
    data_part.setHeader(QNetworkRequest::ContentDispositionHeader, QVariant("form-data; name=\"sioyek_data\""));
    data_part.setBody(json_doc.toJson(QJsonDocument::JsonFormat::Compact));
    multipart->append(data_part);

    multipart->append(image_part);

    QNetworkReply* reply = get_network_manager()->post(req, multipart);
    reply->setParent(parent);
    reply->setProperty("sioyek_network_status_string", "performing query");

    QObject::connect(reply, &QNetworkReply::downloadProgress, [reply, on_chunk=std::move(on_chunk)]() {
        int status_code = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        if (status_code == 200){
            QString chunk = QString::fromUtf8(reply->readAll());
            on_chunk(chunk);
        }
        });
    QObject::connect(reply, &QNetworkReply::finished, [reply, on_done=std::move(on_done)]() {

        int status_code = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        if (status_code == 200) {
            on_done();
        }
        });

}

void SioyekNetworkManager::extract_table_data(QObject* parent, const QPixmap& pixmap, std::function<void(QString)> on_done, std::optional<QString> prompt) {
    QByteArray image_data;
    QBuffer image_buffer(&image_data);
    image_buffer.open(QIODevice::WriteOnly);
    pixmap.save(&image_buffer, "PNG");

    QNetworkRequest req;
    req.setUrl(QUrl(QString::fromStdWString(SIOYEK_HOST + SIOYEK_EXTRACT_TABLE_URL_)));
    authorize_request(&req);

    QHttpMultiPart* multipart = new QHttpMultiPart(QHttpMultiPart::FormDataType);

    QHttpPart image_part;
    image_part.setHeader(QNetworkRequest::ContentTypeHeader, QVariant("image/png"));
    image_part.setHeader(QNetworkRequest::ContentDispositionHeader, QVariant("form-data; name=\"image\"; filename=\"image.png\""));
    image_part.setBody(image_data);

    QJsonDocument json_doc;
    QJsonObject root_object;
    root_object["prompt"] = prompt.value_or(QString::fromStdWString(EXTRACT_TABLE_PROMPT));
    json_doc.setObject(root_object);


    QHttpPart data_part;
    data_part.setHeader(QNetworkRequest::ContentDispositionHeader, QVariant("form-data; name=\"sioyek_data\""));
    data_part.setBody(json_doc.toJson(QJsonDocument::JsonFormat::Compact));
    multipart->append(data_part);

    multipart->append(image_part);

    QNetworkReply* reply = get_network_manager()->post(req, multipart);
    reply->setParent(parent);
    reply->setProperty("sioyek_network_status_string", "extracting data from image");

    QObject::connect(reply, &QNetworkReply::finished, [reply, on_done=std::move(on_done)]() {
        auto data = reply->readAll();
        QJsonObject result_object = QJsonDocument::fromJson(data).object();
        QString status = result_object["status"].toString();
        if (status == "OK") {
            on_done(result_object["result"].toString());
        }
        });


}

void SioyekNetworkManager::delete_annot(QObject* parent, const QString& file_checksum, const QString& annot_type, const QString& uuid, std::function<void()> on_success, std::function<void()> on_fail) {

    QNetworkRequest req;
    req.setUrl(QUrl(QString::fromStdWString(SIOYEK_HOST + SIOYEK_DELETE_ANNOT_URL_)));
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QJsonObject obj;
    obj["uuid"] = uuid;
    obj["file_checksum"] = file_checksum;
    obj["annot_type"] = annot_type;

    QJsonDocument json_doc(obj);

    authorize_request(&req);

    QNetworkReply* reply = get_network_manager()->post(req, json_doc.toJson());
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
    QUrl url(QString::fromStdWString(SIOYEK_HOST + SIOYEK_GET_DOCUMENT_ANNOTATIONS_URL_));
    QUrlQuery params;
    params.addQueryItem("file_checksum", document_checksum);
    url.setQuery(params);

    QNetworkRequest req;
    req.setUrl(url);
    authorize_request(&req);
    auto reply = get_network_manager()->get(req);
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

void SioyekNetworkManager::perform_unsynced_inserts_and_deletes(QObject* parent, Document* doc, const QString& checksum, std::function<void()> on_done) {
    std::vector<std::pair<std::wstring, std::wstring>> unsynced_deletes;

    db_manager->get_all_unsynced_deletions(checksum.toStdString(), unsynced_deletes);

    //Document* doc = document_manager->get_document_with_checksum(checksum.toStdString());
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
        req.setUrl(QUrl(QString::fromStdWString(SIOYEK_HOST + SIOYEK_SYNC_DELETES_URL_)));
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

        QNetworkReply* reply = get_network_manager()->post(req, json_doc.toJson());
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
        return SIOYEK_HOST + SIOYEK_ADD_HIGHLIGHT_URL_;
    }
    if (dynamic_cast<const BookMark*>(annot)) {
        return SIOYEK_HOST + SIOYEK_ADD_BOOKMARK_URL_;
    }
    if (dynamic_cast<const Portal*>(annot)) {
        return SIOYEK_HOST + SIOYEK_ADD_PORTAL_URL_;
    }

    return L"";
}

void SioyekNetworkManager::delete_file_with_checksum(const QString& checksum) {

    if (SERVER_HASHES.find(checksum.toStdString()) != SERVER_HASHES.end()) {
        SERVER_HASHES.erase(checksum.toStdString());
    }

    QNetworkRequest req;
    req.setUrl(QUrl(QString::fromStdWString(SIOYEK_HOST + SIOYEK_DELETE_FILE_URL_)));
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QJsonObject obj;
    obj["file_checksum"] = checksum;

    QJsonDocument json_doc(obj);

    authorize_request(&req);

    QNetworkReply* reply = get_network_manager()->post(req, json_doc.toJson());
}

void SioyekNetworkManager::tts(QObject* parent,
    const std::wstring& text,
    const std::string& document_checksum,
    int page,
    float rate,
    std::function<void(QString, std::vector<float>)> on_done,
    std::function<void(QString)> on_fail
) {
    QString text_checksum = QString::fromStdString(compute_md5_from_data(QString::fromStdWString(text).toUtf8()));
    QString file_path = QString::fromStdWString(cached_tts_path.slash( text_checksum.toStdWString() + L".mp3").get_path());
    QString timestamps_file_path = QString::fromStdWString(cached_tts_path.slash(text_checksum.toStdWString() + L".json").get_path());
    //if (rate != 1) {
    //    file_path  =  QString::fromStdWString(cached_tts_path.slash( text_checksum.toStdWString() + L"_" + QString::number(rate).toStdWString() + L".mp3").get_path());
    //    timestamps_file_path =  QString::fromStdWString(cached_tts_path.slash(text_checksum.toStdWString() + L"_" + QString::number(rate).toStdWString() + L".json").get_path());
    //}
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
        req.setUrl(QUrl(QString::fromStdWString(SIOYEK_HOST + SIOYEK_TTS_URL_)));
        //req.setUrl(QUrl(QString::fromStdWString(SIOYEK_GOOGLE_TTS_URL)));
        req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

        QJsonObject obj;
        obj["text"] = QString::fromStdWString(text);
        obj["text_checksum"] = text_checksum;
        obj["document_checksum"] = QString::fromStdString(document_checksum);
        obj["page"] = page;
        obj["rate"] = rate;

        QJsonDocument json_doc(obj);
        authorize_request(&req);

        QNetworkReply* reply = get_network_manager()->post(req, json_doc.toJson());
        reply->setParent(parent);

        QObject::connect(reply, &QNetworkReply::finished,
            [reply, file_path, text_checksum, timestamps_file_path, on_done = std::move(on_done), on_fail=std::move(on_fail)]() {
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
                on_fail(text_checksum);
            }
            });

    }

}

void SioyekNetworkManager::debug(QObject* parent, std::function<void()> on_done) {

    QNetworkRequest req;
    req.setUrl(QUrl(QString::fromStdWString(SIOYEK_HOST + SIOYEK_DEBUG_URL_)));
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QJsonObject obj;
    obj["secs"] = 4;

    QJsonDocument json_doc(obj);
    authorize_request(&req);

    QNetworkReply* reply = get_network_manager()->post(req, json_doc.toJson());
    reply->setParent(parent);
    reply->setProperty("sioyek_network_status_string", "Peforming debug operation");
    QObject::connect(reply, &QNetworkReply::finished, [on_done=std::move(on_done)]() {
        on_done();
        });

}

void SioyekNetworkManager::semantic_search_extractive(QObject* parent, const QString& query, const std::wstring& index, std::function<void(QJsonObject response)> on_done) {
    QNetworkRequest req;
    req.setUrl(QUrl(QString::fromStdWString(SIOYEK_HOST + SIOYEK_SEMANTIC_ASK_GEMINI_URL_)));
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QJsonObject obj;
    obj["document_content"] = QString::fromStdWString(index);
    obj["query"] = query;

    QJsonDocument json_doc(obj);
    authorize_request(&req);

    QNetworkReply* reply = get_network_manager()->post(req, json_doc.toJson());
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

void SioyekNetworkManager::semantic_search(QObject* parent, const QString& query, const std::wstring& index, std::function<void(QJsonObject response)> on_done) {
    QNetworkRequest req;
    req.setUrl(QUrl(QString::fromStdWString(SIOYEK_HOST + SIOYEK_SEMANTIC_SEARCH_URL_)));
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QJsonObject obj;
    obj["document_content"] = QString::fromStdWString(index);
    obj["query"] = query;

    QJsonDocument json_doc(obj);
    authorize_request(&req);

    QNetworkReply* reply = get_network_manager()->post(req, json_doc.toJson());
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

void SioyekNetworkManager::does_index_exist(QObject* parent, const std::wstring& index, std::function<void(bool)> on_done) {
    QString index_qstring = QString::fromStdWString(index);
    std::string content_checksum = compute_md5_from_data(index_qstring.toUtf8());

    QNetworkRequest req;
    req.setUrl(QUrl(QString::fromStdWString(SIOYEK_HOST + SIOYEK_DOES_INDEX_EXIST_URL_)));
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QJsonObject obj;
    obj["content_checksum"] = QString::fromStdString(content_checksum);

    QJsonDocument json_doc(obj);
    authorize_request(&req);

    QNetworkReply* reply = get_network_manager()->post(req, json_doc.toJson());
    reply->setParent(parent);
    reply->setProperty("sioyek_network_status_string", "searching for document index");
    QObject::connect(reply, &QNetworkReply::finished, [reply, on_done=std::move(on_done)]() {
        int status_code = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        if (status_code == 200) {
            auto json_doc = QJsonDocument::fromJson(reply->readAll());
            if (json_doc["status"].toString() == "EXISTS") {
                on_done(true);
            }
            else {
                on_done(false);
            }
        }
        });
}

void SioyekNetworkManager::semantic_ask(QObject* parent, const QString& query, const std::wstring& index, int first_page_end_index, std::function<void(QString)> on_chunk, std::function<void()> on_done) {
    QNetworkRequest req;
    req.setUrl(QUrl(QString::fromStdWString(SIOYEK_HOST + SIOYEK_SEMANTIC_ASK_URL_)));
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QJsonObject obj;
    obj["document_content"] = QString::fromStdWString(index);
    obj["first_page_end_index"] = first_page_end_index;
    obj["query"] = query;

    QJsonDocument json_doc(obj);
    authorize_request(&req);

    QNetworkReply* reply = get_network_manager()->post(req, json_doc.toJson());
    reply->setParent(parent);
    reply->setProperty("sioyek_network_status_string", "performing semantic search");
    QObject::connect(reply, &QNetworkReply::downloadProgress, [reply, on_chunk=std::move(on_chunk)]() {
        int status_code = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        if (status_code == 200){
            QString chunk = QString::fromUtf8(reply->readAll());
            on_chunk(chunk);
        }
        });
    QObject::connect(reply, &QNetworkReply::finished, [reply, on_done=std::move(on_done)]() {
        int status_code = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        if (status_code == 200) {
            on_done();
        }
        });
}

void SioyekNetworkManager::summarize(QObject* parent, const std::wstring& index, int first_page_end_index, std::function<void(QString)> on_chunk, std::function<void()> on_done) {
    QString index_qstring = QString::fromStdWString(index);
    //std::string content_checksum = compute_md5_from_data(index_qstring.toUtf8());

    QNetworkRequest req;
    req.setUrl(QUrl(QString::fromStdWString(SIOYEK_HOST + SIOYEK_SUMMARIZE_URL_)));
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QJsonObject obj;
    obj["document_content"] = index_qstring;
    obj["first_page_end_index"] = first_page_end_index;

    QJsonDocument json_doc(obj);
    authorize_request(&req);

    QNetworkReply* reply = get_network_manager()->post(req, json_doc.toJson());
    reply->setParent(parent);
    reply->setProperty("sioyek_network_status_string", "summarizing");
    QObject::connect(reply, &QNetworkReply::downloadProgress, [reply, on_chunk=std::move(on_chunk)]() {
        int status_code = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

        if (status_code == 200) {
            QString chunk = QString::fromUtf8(reply->readAll());
            on_chunk(chunk);
        }

        });
    QObject::connect(reply, &QNetworkReply::finished, [reply, on_done=std::move(on_done)]() {
        int status_code = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

        if (status_code == 200) {
            on_done();
        }
        else {
            if (status_code == 400) {
                auto content = reply->readAll();
                QJsonDocument json_doc = QJsonDocument::fromJson(content);
                QString status_string = json_doc["status"].toString();
                if (status_string == "TOO_LONG") {
                    show_error_message(L"Document is too long for summarization");
                }

            }
        }

        });
}

void SioyekNetworkManager::upload_document_index(QObject* parent, const std::wstring& document_content, std::function<void(QJsonObject)> on_done) {
    QString index_qstring = QString::fromStdWString(document_content);
    std::string content_checksum = compute_md5_from_data(index_qstring.toUtf8());

    QNetworkRequest req;
    req.setUrl(QUrl(QString::fromStdWString(SIOYEK_HOST + SIOYEK_UPLOAD_INDEX_URL_)));
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QJsonObject obj;
    obj["content_checksum"] = QString::fromStdString(content_checksum);
    obj["text"] = index_qstring;

    QJsonDocument json_doc(obj);
    authorize_request(&req);

    QNetworkReply* reply = get_network_manager()->post(req, json_doc.toJson());
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

void SioyekNetworkManager::delete_file_from_server(QObject* parent, std::string checksum, std::function<void()> on_done) {

    QNetworkRequest req;
    req.setUrl(QUrl(QString::fromStdWString(SIOYEK_HOST + SIOYEK_DELETE_FILE_CHECKSUM_URL_)));
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QJsonObject obj;
    obj["file_checksum"] = QString::fromStdString(checksum);

    QJsonDocument json_doc(obj);

    authorize_request(&req);

    QNetworkReply* reply = get_network_manager()->post(req, json_doc.toJson());
    reply->setParent(parent);
    QObject::connect(reply, &QNetworkReply::finished, [this, reply, checksum, on_done=std::move(on_done)]() {
        int status_code = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        if (status_code != 200) {
        }
        else {
            if (SERVER_HASHES.find(checksum) != SERVER_HASHES.end()) {
                SERVER_HASHES.erase(checksum);
            }
            on_done();
        }
        });
}


QNetworkReply* SioyekNetworkManager::upload_drawings(QObject* parent, std::string pdf_file_checksum, std::wstring drawing_file_path, std::function<void()> on_done) {

    QFile* file = new QFile(QString::fromStdWString(drawing_file_path));
    if (file->open(QIODevice::ReadOnly)) {
        QHttpMultiPart* parts = new QHttpMultiPart(QHttpMultiPart::FormDataType);

        QJsonDocument json_doc;
        QJsonObject root_object;
        root_object["sioyek_version"] = QString::fromStdString(APPLICATION_VERSION);
        root_object["file_checksum"] = QString::fromStdString(pdf_file_checksum);
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
        req.setUrl(QUrl(QString::fromStdWString(SIOYEK_HOST + SIOYEK_UPLOAD_DRAWINGS_URL_)));

        file->setParent(parts);
        // DO NOT use the Qt's default boundary, it does not work
        parts->setBoundary(create_random_string().toUtf8());
        req.setHeader(QNetworkRequest::KnownHeaders::ContentTypeHeader, "multipart/form-data; boundary=" + parts->boundary());

        QNetworkReply* reply = get_network_manager()->post(req, parts);
        reply->setProperty("sioyek_handled", true);
        reply->setParent(parent);

        QObject::connect(reply, &QNetworkReply::finished, [this, reply, on_done=std::move(on_done)]() {

            if (handle_network_reply_if_error(reply, true)) {
                //update_user_files_hash_set();
                auto json_resp = QJsonDocument::fromJson(reply->readAll());
                if (json_resp["status"] != "OK" && json_resp["type"] == "incorrect_file_hash") {
                    //todo: update_current_document_checksum()

                    show_error_message(L"the file hash was incorrect");
                }
                else {
                    on_done();
                }
            }
            });

        parts->setParent(reply);
        return reply;
    }
    return nullptr;
}

void SioyekNetworkManager::get_last_drawing_modification_time(QObject* parent, std::string pdf_file_checksum, std::function<void(std::optional<QDateTime>)> on_done) {

    QNetworkRequest req;
    req.setUrl(QUrl(QString::fromStdWString(SIOYEK_HOST + SIOYEK_GET_LAST_DRAWING_MODIFICATION_TIME_URL_)));
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QJsonObject obj;
    obj["file_checksum"] = QString::fromStdString(pdf_file_checksum);

    QJsonDocument json_doc(obj);

    authorize_request(&req);

    QNetworkReply* reply = get_network_manager()->post(req, json_doc.toJson());
    reply->setParent(parent);
    QObject::connect(reply, &QNetworkReply::finished, [this, reply, pdf_file_checksum, on_done=std::move(on_done)]() {
        int status_code = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        if (status_code != 200) {
        }
        else {
            QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
            QJsonObject root = doc.object();
            if (root["status"].toString() == "OK") {
                on_done(QDateTime::fromString(root["last_modification_time"].toString(), Qt::ISODate));
            }
            else {
                // not found
                on_done({});
            }
        }
        });
}

void SioyekNetworkManager::download_drawings(QObject* parent, std::string checksum, std::wstring target_path, std::function<void()> on_done) {

    QNetworkRequest req;
    req.setUrl(QUrl(QString::fromStdWString(SIOYEK_HOST + SIOYEK_DOWNLOAD_DRAWINGS_URL_)));
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QJsonObject obj;
    obj["file_checksum"] = QString::fromStdString(checksum);

    QJsonDocument json_doc(obj);

    authorize_request(&req);

    QNetworkReply* reply = get_network_manager()->post(req, json_doc.toJson());
    reply->setParent(parent);
    QObject::connect(reply, &QNetworkReply::finished, [reply, target_path, on_done=std::move(on_done)]() {
        int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        if (status != 200) {
        }
        else {
            QFile file(QString::fromStdWString(target_path));
            if (file.open(QIODevice::WriteOnly)) {
                file.write(reply->readAll());
                file.close();
            }
            on_done();
        }
        });

}

void SioyekNetworkManager::download_new_annotations(QObject* parent, QDateTime last_update_time, bool force) {
    if (force || !already_downloaded_new_annotations) {
        already_downloaded_new_annotations = true;
        get_annotations_after(parent, last_update_time, [this, parent](std::vector<std::pair<std::string, Highlight>>&& highlights, std::vector<std::pair<std::string, BookMark>>&& bookmarks, std::vector<std::pair<std::string, Portal>>&& portals) {
            background_task_manager->add_task([this, highlights = std::move(highlights), bookmarks = std::move(bookmarks), portals = std::move(portals)]() {

                for (auto& [file_checksum, highlight] : highlights) {
                    db_manager->insert_or_update_highlight_synced(true, file_checksum, highlight);
                }
                for (auto& [file_checksum, bookmark] : bookmarks) {
                    db_manager->insert_or_update_bookmark_synced(true, file_checksum, bookmark);
                }
                for (auto& [file_checksum, portal] : portals) {
                    db_manager->insert_or_update_portal_synced(true, file_checksum, portal);
                }
                }, parent);
            });
    }
}

void SioyekNetworkManager::get_annotations_after(QObject* parent, QDateTime last_update_date, std::function<void(std::vector<std::pair<std::string, Highlight>>&&, std::vector<std::pair<std::string, BookMark>>&&, std::vector<std::pair<std::string, Portal>>&&)> fn) {

    QUrl url(QString::fromStdWString(SIOYEK_HOST + SIOYEK_GET_NEW_ANNOTATIONS_URL_));

    QJsonObject obj;
    obj["after_date"] = last_update_date.toString(Qt::ISODate);

    QJsonDocument json_doc(obj);

    QNetworkRequest req;
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    req.setUrl(url);
    authorize_request(&req);
    auto reply = get_network_manager()->post(req, json_doc.toJson());
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

            std::vector<std::pair<std::string, Highlight>> res_highlights;
            std::vector<std::pair<std::string,BookMark>> res_bookmarks;
            std::vector<std::pair<std::string, Portal>> res_portals;

            for (int i = 0; i < json_highlights.size(); i++) {
                QJsonObject json_highlight = json_highlights[i].toObject();

                Highlight highlight = Highlight::from_json(json_highlight);
                res_highlights.push_back(std::make_pair(json_highlight["file_checksum"].toString().toStdString(), highlight));
            }
            for (int i = 0; i < json_bookmarks.size(); i++) {
                QJsonObject json_bookmark = json_bookmarks[i].toObject();

                BookMark bookmark = BookMark::from_json(json_bookmark);
                res_bookmarks.push_back(std::make_pair(json_bookmark["file_checksum"].toString().toStdString(), bookmark));
            }

            for (int i = 0; i < json_portals.size(); i++) {
                QJsonObject json_portal = json_portals[i].toObject();

                Portal portal = Portal::from_json(json_portal);
                res_portals.push_back(std::make_pair(json_portal["file_checksum"].toString().toStdString(), portal));
            }

            fn(std::move(res_highlights), std::move(res_bookmarks), std::move(res_portals));

        }

        });
}

void block_for_send(QNetworkReply* reply) {
    QEventLoop loop;
    QObject::connect(reply, &QNetworkReply::requestSent, &loop, &QEventLoop::quit);
    loop.exec();
}

void block_for_sends(std::vector<QNetworkReply*> reply) {
    std::vector<QEventLoop> loops(reply.size());
    for (int i = 0; i < reply.size(); ++i) {
        QObject::connect(reply[i], &QNetworkReply::requestSent, &loops[i], &QEventLoop::quit);
    }
    for (auto& l : loops) {
        l.exec();
    }

}

void SioyekNetworkManager::sync_document_annotations_to_server(QObject* parent, Document* doc, std::function<void()> on_done) {
    if (!doc) return;
    if (!doc->get_checksum_fast()) return;
    if (!(status == ServerStatus::LoggedIn)) return;
    if ((!doc->get_is_synced())) {
        download_annotations_since_last_sync();
        return;
    }

    QString document_checksum = QString::fromStdString(doc->get_checksum_fast().value());
    get_last_drawing_modification_time(parent, doc->get_checksum_fast().value(), [this, parent, document=doc](std::optional<QDateTime> server_modification_time) {
        std::optional<QDateTime> local_modification_time = document->get_local_drawings_modification_time();
        // if the server file is newer, update the local file
        if (server_modification_time.has_value() && local_modification_time.has_value()) {
            qDebug() << local_modification_time.value().secsTo(server_modification_time.value());
        }
        if (server_modification_time.has_value() &&
            (!local_modification_time.has_value() ||
                (local_modification_time.value().secsTo(server_modification_time.value()) > 10)
                )) {
            download_drawings(parent, document->get_checksum_fast().value(), document->get_drawings_file_path(), [this, document]() {
                document->load_drawings();
                });
        }
        });

    perform_unsynced_inserts_and_deletes(parent, doc, document_checksum, [this, document_checksum, parent, document=doc, on_done=std::move(on_done)]() {
        get_document_annotations(parent, document_checksum,
        [&, document_checksum, document, on_done=std::move(on_done)](std::vector<Highlight>&& server_highlights, std::vector<BookMark>&& server_bookmarks, std::vector<Portal>&& server_portals, std::optional<QDateTime> server_access_time) {

                const std::vector<Highlight>& local_highlights = document->get_highlights();
                const std::vector<BookMark>& local_bookmarks = document->get_bookmarks();
                const std::vector<Portal>& local_portals = document->get_portals();

                auto [local_only_highlights, server_only_highlights, intersection_highlights] = decompose_sets(local_highlights, server_highlights);
                auto [local_only_bookmarks, server_only_bookmarks, intersection_bookmarks] = decompose_sets(local_bookmarks, server_bookmarks);
                auto [local_only_portals, server_only_portals, intersection_portals] = decompose_sets(local_portals, server_portals);

                auto sync_annot_intersection = [&, this, document_checksum=document->get_checksum_fast()](auto intersection) {
                    Document* synced_doc = document_manager->get_document_with_checksum(document_checksum.value());
                    for (const auto& [local_annot, server_annot] : intersection) {
                        // sync only if the annotation has changed
                        if (has_changed(local_annot, server_annot)) {
                            QDateTime local_modification_time = QDateTime::fromString(QString::fromStdString(local_annot.modification_time), Qt::ISODate);
                            QDateTime server_modification_time = QDateTime::fromString(QString::fromStdString(server_annot.modification_time), Qt::ISODate);
                            local_modification_time.setTimeSpec(Qt::UTC);
                            server_modification_time.setTimeSpec(Qt::UTC);
                            if (server_modification_time > local_modification_time) {
                                // server is the authority
                                //db_manager->update_annot_with_server_annot(&server_annot);
                                synced_doc->update_annotation_with_server_annotation(&server_annot);
                            }
                            else {
                                // todo: this should be batched
                                upload_annot(parent,
                                    QString::fromStdString(document->get_checksum_fast().value()),
                                    local_annot,
                                    []() {
                                    },
                                    []() {
                                    }
                                );
                            }
                        }
                    }
                    };

                sync_annot_intersection(intersection_highlights);
                sync_annot_intersection(intersection_bookmarks);
                sync_annot_intersection(intersection_portals);


                //// todo: this should be batched
                document->lock_highlights_mutex();
                for (const auto& local_highlight : local_only_highlights) {
                    document->delete_highlight_with_uuid(local_highlight.uuid, true);
                }
                document->unlock_highlights_mutex();

                for (const auto& local_bookmark : local_only_bookmarks) {
                    document->delete_bookmark_with_uuid(local_bookmark.uuid, true);
                }

                for (const auto& local_portal : local_only_portals) {
                    document->delete_portal_with_uuid(local_portal.uuid, true);
                }

                // todo: this should be batched
                for (const auto& server_highlight : server_only_highlights) {
                    document->add_highlight_with_existing_uuid(server_highlight);
                }

                for (const auto& server_bookmark : server_only_bookmarks) {
                    document->add_bookmark_with_existing_uuid(server_bookmark);
                }

                for (const auto& server_portal : server_only_portals) {
                    document->add_portal_with_existing_uuid(server_portal);
                }
                download_annotations_since_last_sync();


                on_done();
                //invalidate_render();
            });

        });


}

void SioyekNetworkManager::download_annotations_since_last_sync(bool force_all) {
    QDateTime last_annotation_update_time = get_last_server_sync_time().value_or(QDateTime::currentDateTimeUtc().addYears(-100));
    save_last_server_sync_time();
    if (force_all) {
        last_annotation_update_time = last_annotation_update_time.addYears(-100);
    }
    download_new_annotations(nullptr, last_annotation_update_time, force_all);
}

std::optional<QDateTime> SioyekNetworkManager::get_last_server_sync_time() {
    if (!last_server_sync_time.has_value()) {
        std::optional<QJsonObject> obj = get_sioyek_json_data();
        if (obj) {
            last_server_sync_time = QDateTime::fromString(obj.value()["sync_time"].toString(), Qt::ISODate);
        }
    }
    return last_server_sync_time;
}

void SioyekNetworkManager::save_last_server_sync_time() {
    QDateTime current_time = QDateTime::currentDateTime().toUTC();
    QJsonObject root;
    root["sync_time"] = current_time.toString(Qt::ISODate);
    QJsonDocument json_doc(root);

    QFile json_file(QString::fromStdWString(sioyek_json_data_path.get_path()));
    json_file.open(QFile::WriteOnly);
    json_file.write(json_doc.toJson());
    json_file.close();
}

std::optional<QJsonObject> SioyekNetworkManager::get_sioyek_json_data() {
    if (!sioyek_json_data.has_value()) {
        QFile file(QString::fromStdWString(sioyek_json_data_path.get_path()));
        if (file.exists() && file.open(QIODeviceBase::ReadOnly)) {
            QJsonDocument json_document = QJsonDocument::fromJson(file.readAll());
            sioyek_json_data = json_document.object();
            file.close();
        }
    }
    return sioyek_json_data;
}

void SioyekNetworkManager::sync_deleted_annot(QObject* parent, Document* doc, const std::string& annot_type, const std::string& uuid) {
    if (is_document_available_on_server(doc)) {
        auto checksum = doc->get_checksum_fast();
        if (checksum) {
            delete_annot(
                parent,
                QString::fromStdString(checksum.value()),
                QString::fromStdString(annot_type),
                QString::fromStdString(uuid),
                [this, uuid, checksum]() { // on success
                    //db_manager->set_highlight_uuid_to_synced(uuid);

                },
                [this, uuid, checksum, annot_type]() { // on fail
                    db_manager->insert_unsynced_deletion(annot_type, uuid, checksum.value());
                }
            );
        }
    }
    else {
        auto checksum = doc->get_checksum_fast();
        if (checksum && doc->get_is_synced()) {
            db_manager->insert_unsynced_deletion(annot_type, uuid, checksum.value());
        }
    }
    doc->update_last_local_edit_time();
}

bool SioyekNetworkManager::is_document_available_on_server(Document* doc) {
    if (!doc) {
        return false;
    }
    
    std::optional<std::string> maybe_checksum = doc->get_checksum_fast();
    if (maybe_checksum.has_value()) {
        return is_checksum_available_on_server(maybe_checksum.value());
    }
    else {
        return false;
    }
}

void SioyekNetworkManager::search_all_documents(QObject* parent, QString q, std::function<void(std::vector<QString>)> on_done){
    QUrl url(QString::fromStdWString(SIOYEK_HOST + SIOYEK_API_SEARCH_URL_));
    QUrlQuery query;
    query.addQueryItem("query", "a " + q);
    url.setQuery(query);


    QNetworkRequest req;
    authorize_request(&req);
    req.setUrl(url);
    QNetworkReply* reply = get_network_manager()->get(req);
    reply->setProperty("sioyek_handled", true);
    reply->setParent(parent);
    QObject::connect(reply, &QNetworkReply::finished, [this, reply, q, on_done = std::move(on_done)]() {
        auto json_doc = QJsonDocument::fromJson(reply->readAll());
        QJsonArray results = json_doc.object()["results"].toArray();
        std::vector<QString> matches;

        for (int i = 0; i < results.size(); i++) {
            QString match = results[i].toObject()["obj"].toObject()["match"].toString();
            matches.push_back(match);
        }
        on_done(matches);
        });
}

bool SioyekNetworkManager::should_sync_document_to_server(Document *doc) {

    if (doc) {
        return doc->get_is_synced();
    }
    return false;
}

QString SioyekNetworkManager::get_login_status_string(Document *current_document) {
    QString server_status_string;

    if (status == ServerStatus::LoggedIn) {
        if (is_document_available_on_server(current_document)) {
            if (current_document->get_is_synced()) {
                server_status_string = "SYNCED";
            }
            else {
                server_status_string = "DESYNCHRONIZED";
            }
        }
        else {
            if (should_sync_document_to_server(current_document)) {
                server_status_string = "SYNC PENDING";
            }
            else {
                server_status_string = "UNSYNCED";
            }
        }
    }
    else {
        if (network_manager_ == nullptr) {
            server_status_string = "OFFLINE";
        }
        else if (status == ServerStatus::NotLoggedIn) {
            server_status_string = "NOT LOGGED IN";
        }
        else if (status == ServerStatus::ServerOffline) {
            server_status_string = "SERVER OFFLINE";
        }
        else if (status == ServerStatus::InvalidCredentials) {
            server_status_string = "EXPIRED/INVALID CREDENTIALS";
        }
        else if (status == ServerStatus::LoggingIn) {
            server_status_string = "LOGGING IN";
        }
    }

    return "[ " + server_status_string + " ]";
}

void SioyekNetworkManager::handle_logout() {
    ACCESS_TOKEN = "";
    persist_access_token(ACCESS_TOKEN);
    one_time_network_operations_performed = false;
    SERVER_HASHES.clear();
    server_opened_files.clear();
    last_server_location.clear();
    status = ServerStatus::NotLoggedIn;
}

void SioyekNetworkManager::cancel_all_downlods() {
    QList<QNetworkReply*> children = network_manager_->findChildren<QNetworkReply*>();
    for (auto child : children) {
        child->abort();
    }
}
