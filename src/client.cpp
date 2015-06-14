#include <client.h>

#include <core/net/error.h>
#include <core/net/http/client.h>
#include <core/net/http/content_type.h>
#include <core/net/http/response.h>
#include <QVariantMap>
#include <QDebug>

namespace http = core::net::http;
namespace net = core::net;

using namespace std;

Client::Client(Config::Ptr config) :
    config_(config), cancelled_(false) {
}


void Client::get(const net::Uri::Path &path,
                 const net::Uri::QueryParameters &parameters, QJsonDocument &root) {
    // Create a new HTTP client
    auto client = http::make_client();

    // Start building the request configuration
    http::Request::Configuration configuration;

    // Build the URI from its components
    net::Uri uri = net::make_uri(config_->apiroot, path, parameters);
    configuration.uri = client->uri_to_string(uri);

    // Give out a user agent string
    configuration.header.add("User-Agent", config_->user_agent);

    // Build a HTTP request object from our configuration
    auto request = client->head(configuration);

    try {
        // Synchronously make the HTTP request
        // We bind the cancellable callback to #progress_report
        auto response = request->execute(
                    bind(&Client::progress_report, this, placeholders::_1));

        // Check that we got a sensible HTTP status code
        if (response.status != http::Status::ok) {
            throw domain_error(response.body);
        }
        // Parse the JSON from the response
        root = QJsonDocument::fromJson(response.body.c_str());

        // Open weather map API error code can either be a string or int
        QVariant cod = root.toVariant().toMap()["cod"];
        if ((cod.canConvert<QString>() && cod.toString() != "200")
                || (cod.canConvert<unsigned int>() && cod.toUInt() != 200)) {
            throw domain_error(root.toVariant().toMap()["message"].toString().toStdString());
        }
    } catch (net::Error &) {
    }
}

Client::VideoList Client::getVideosByCategory(const string &category,
                                              const string &keyword,
                                              const string &period,
                                              const string &orderby,
                                              const string &count) {

    // In this case we are going to retrieve JSON data.
    QJsonDocument root;

    if (keyword.empty()) {
        // 没有关键字，使用 根据分类获取视频 接口
        get(
            { "v2", "videos", "by_category.json" },
            { { "client_id", config_->client_id }, {"category", category}, {"period", period}, {"count", count} },
            root);
        // e.g. https://openapi.youku.com/v2/videos/by_category.json?client_id=e38f2f239754dc06
    } else {
        // 有关键字，使用 搜索视频通过关键词 接口
        get(
            { "v2", "searches", "video", "by_keyword.json" },
            { { "client_id", config_->client_id }, { "keyword", keyword }, {"category", category}, {"period", period}, {"orderby", orderby}, {"count", count} },
            root);
        // e.g. https://openapi.youku.com/v2/searches/video/by_keyword.json?client_id=e38f2f239754dc06
    }


    VideoList videoList;

    // 读取视频 Json
    QVariantMap variant = root.toVariant().toMap();
    for (const QVariant &i : variant["videos"].toList()) {
        QVariantMap item = i.toMap();

        videoList.emplace_back(
            Video {
                item["id"].toString().toStdString(),
                item["title"].toString().toStdString(),
                item["link"].toString().toStdString(),
                item["thumbnail"].toString().toStdString(),
                item["description"].toString().toStdString(),
                item["view_count"].toString().toStdString(),
                item["up_count"].toString().toStdString(),
                item["down_count"].toString().toStdString(),
                item["published"].toString().toStdString()
            }
        );
    }

    return videoList;
}

Client::ShowList Client::getShowsByCategory(const string &category, const string &keyword, const string &period, const string &count) {

    // In this case we are going to retrieve JSON data.
    QJsonDocument root;


    if (keyword.empty()) {
        get(
            { "v2", "shows", "by_category.json" },
            { { "client_id", config_->client_id }, {"category", category}, {"period", period}, {"count", count} },
            root);
        // e.g. https://openapi.youku.com/v2/videos/by_category.json?client_id=e38f2f239754dc06
    } else {
        get(
            { "v2", "searches", "show", "by_keyword.json" },
            { { "client_id", "e38f2f239754dc06" }, {"keyword", keyword}, {"category", category}, {"period", period}, {"count", count} },
            root);
        // e.g. https://openapi.youku.com/v2/searches/show/by_keyword.json?client_id=e38f2f239754dc06
    }

    ShowList showList;

    // Json
    QVariantMap variant = root.toVariant().toMap();
    for (const QVariant &i : variant["shows"].toList()) {
        QVariantMap item = i.toMap();

        showList.emplace_back(
            Show {
                item["id"].toString().toStdString(),
                item["name"].toString().toStdString(),
                item["link"].toString().toStdString(),
                item["thumbnail"].toString().toStdString(),
                item["episode_updated"].toString().toStdString(),
                item["view_count"].toString().toStdString(),
                item["score"].toString().toStdString(),
                item["published"].toString().toStdString()
            }
        );
    }

    return showList;
}

// 视频详情
Client::VideoDetail Client::getVideoDetailById(const string &video_id) {

    QJsonDocument root;

    get(
        { "v2", "videos", "show.json" },
        {{ "client_id", config_->client_id }, {"video_id", video_id}},
        root);

    Client::VideoDetail vDetail;
    QVariantMap variant = root.toVariant().toMap();

    qDebug() << "[视频详情]" << variant["id"].toString() << variant["title"].toString();

    vDetail = VideoDetail {
        variant["id"].toString().toStdString(),
        variant["title"].toString().toStdString(),
        variant["link"].toString().toStdString(),
        variant["thumbnail"].toString().toStdString(),
        variant["bigThumbnail"].toString().toStdString(),
        (int)variant["duration"].toDouble(),
        variant["category"].toString().toStdString(),
        variant["published"].toString().toStdString(),
        variant["description"].toString().toStdString(),
        variant["player"].toString().toStdString(),
        variant["tags"].toString().toStdString(),
        variant["view_count"].toString().toInt(),
        variant["favorite_count"].toString().toInt(),
        variant["comment_count"].toString().toInt(),
        variant["up_count"].toString().toInt(),
        variant["down_count"].toString().toInt(),
    };

    return vDetail;
}

// 节目详情
Client::ShowDetail Client::getShowDetailById(const string &show_id) {
    QJsonDocument root;

    get(
        { "v2", "shows", "show.json" },
        {{ "client_id", config_->client_id }, {"show_id", show_id}},
        root);

    Client::ShowDetail sDetail;

    QVariantMap variant = root.toVariant().toMap();

    qDebug() << "[节目详情]" << variant["id"].toString() << variant["name"].toString();

    sDetail = ShowDetail {
        variant["id"].toString().toStdString(),
        variant["name"].toString().toStdString(),
        variant["alias"].toString().toStdString(),
        variant["link"].toString().toStdString(),
        variant["play_link"].toString().toStdString(),
        variant["poster"].toString().toStdString(),
        variant["poster_large"].toString().toStdString(),
        variant["thumbnail"].toString().toStdString(),
        variant["genre"].toString().toStdString(),
        variant["area"].toString().toStdString(),
        variant["episode_count"].toString().toInt(),
        variant["episode_updated"].toString().toInt(),
        variant["view_count"].toInt(),
        variant["score"].toFloat(),
        variant["released"].toString().toStdString(),
        variant["category"].toString().toStdString(),
        variant["description"].toString().toStdString(),
        variant["rank"].toString().toInt(),
        variant["view_yesterday_count"].toString().toInt(),
        variant["view_week_count"].toString().toInt(),
        variant["comment_count"].toString().toInt(),
        variant["favorite_count"].toString().toInt(),
        variant["up_count"].toString().toInt(),
        variant["down_count"].toString().toInt(),
    };

    return sDetail;
}


http::Request::Progress::Next Client::progress_report(
        const http::Request::Progress&) {

    return cancelled_ ?
                http::Request::Progress::Next::abort_operation :
                http::Request::Progress::Next::continue_operation;
}

void Client::cancel() {
    cancelled_ = true;
}

Client::Config::Ptr Client::config() {
    return config_;
}

