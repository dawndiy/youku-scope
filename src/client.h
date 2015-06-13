#ifndef CLIENT_H_
#define CLIENT_H_

#include <atomic>
#include <deque>
#include <map>
#include <memory>
#include <string>
#include <core/net/http/request.h>
#include <core/net/uri.h>

#include <QJsonDocument>

/**
 * Provide a nice way to access the HTTP API.
 *
 * We don't want our scope's code to be mixed together with HTTP and JSON handling.
 */
class Client {
public:

    /**
     * Client configuration
     */
    struct Config {
        typedef std::shared_ptr<Config> Ptr;

        // The root of all API request URLs
        std::string apiroot { "https://openapi.youku.com" };

        // The custom HTTP user agent string for this library
        std::string user_agent { "example-network-scope 0.1; (foo)" };

        // YouKu App Info
        std::string client_id {"7260506159958ac8"};
        std::string client_secret {"0637808b218fa55046aed63c6ef8dae1"};
    };

    /**
     * 视频基础信息
     */
    struct Video {
       std::string id;
       std::string title;
       std::string link;
       std::string thumbnail;
       std::string description;
       std::string view_count;
       std::string up_count;
       std::string down_count;
       std::string published;
    };

    /**
     * 视频详细信息
     */
    struct VideoDetail {
        std::string id;
        std::string title;
        std::string link;
        std::string thumbnail;
        std::string bigThumbnail;
        int duration;
        std::string category;
        std::string published;
        std::string description;
        std::string player;
        std::string tags;
        int view_count;
        int favorite_count;
        int comment_count;
        int up_count;
        int down_count;
    };

    typedef std::deque<Video> VideoList;

    /**
     * 根据分类获取视频
     */
    virtual VideoList getVideosByCategory(
            const std::string &category="",
            const std::string &keyword="",
            const std::string &period="today",
            const std::string &orderby="relevance",
            const std::string &count="50");

    /**
     * 根据视频ID获取视频详细信息
     */
    virtual VideoDetail getVideoDetailById(const std::string &video_id);

    /**
     * 视频分类信息
     */
    struct Category {
        std::string term;
        std::string label;
        std::string lang;
    };

    typedef std::deque<Category> CategoryList;

    /**
     * 节目基础信息
     */
    struct Show {
        std::string id;
        std::string name;
        std::string link;
        std::string thumbnail;
        std::string episode_updated;
        std::string view_count;
        std::string score;
        std::string published;
    };

    /**
     * 节目详细信息
     */
    struct ShowDetail {
        std::string id;
        std::string name;
        std::string alias;
        std::string link;
        std::string play_link;          // 允许为空
        std::string poster;             // 允许为空
        std::string poster_large;       // 允许为空
        std::string thumbnail;
        std::string genre;
        std::string area;
        int episode_count;
        int episode_updated;            // 允许为空
        int view_count;
        float score;
        std::string released;
        std::string category;
        std::string description;
        int rank;
        int view_yesterday_count;
        int view_week_count;
        int comment_count;
        int favorite_count;
        int up_count;
        int down_count;
    };

    typedef std::deque<Show> ShowList;

    /**
     * 根据粉来获取节目
     */
    virtual ShowList getShowsByCategory(const std::string &category="电视剧", const std::string &keyword="", const std::string &period="today", const std::string &count="50");

    /**
     * 根据节目ID获取节目详细信息
     */
    virtual ShowDetail getShowDetailById(const std::string &show_id);

    Client(Config::Ptr config);

    virtual ~Client() = default;

    /**
     * Cancel any pending queries (this method can be called from a different thread)
     */
    virtual void cancel();

    virtual Config::Ptr config();

protected:
    void get(const core::net::Uri::Path &path,
             const core::net::Uri::QueryParameters &parameters,
             QJsonDocument &root);
    /**
     * Progress callback that allows the query to cancel pending HTTP requests.
     */
    core::net::http::Request::Progress::Next progress_report(
            const core::net::http::Request::Progress& progress);

    /**
     * Hang onto the configuration information
     */
    Config::Ptr config_;

    /**
     * Thread-safe cancelled flag
     */
    std::atomic<bool> cancelled_;
};

#endif // CLIENT_H_

