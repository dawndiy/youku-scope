#include <query.h>
#include <localization.h>

#include <unity/scopes/Annotation.h>
#include <unity/scopes/CategorisedResult.h>
#include <unity/scopes/CategoryRenderer.h>
#include <unity/scopes/QueryBase.h>
#include <unity/scopes/SearchReply.h>

#include <iomanip>
#include <ctime>
#include <QFile>
#include <QIODevice>

#include <QDebug>

namespace sc = unity::scopes;

using namespace std;


// result orderby
QList<string> resultOrderby = {"relevance", "published", "view-count"};
// Show Categories
QList<string> indexTopShowCategories = {"综艺", "电视剧", "电影", "动漫", "体育"};


/*****************************************
 * TEMPLATES
 *****************************************/
const static string VIDEO_CAROUSEL = R"({
    "schema-version": 1,
    "template": {
        "category-layout": "carousel",
        "card-size": "medium",
        "overlay" : true
    },
    "components": {
        "title": "title",
        "art" : {
            "field": "art",
            "fill-mode": "fit",
            "aspect-ratio": 0.98
        },
        "emblem": "source_log",
        "attributes": "attributes"
    }
})";

const static string VIDEO_GRID = R"({
    "schema-version": 1,
    "template": {
        "category-layout": "grid",
        "card-layout": "vertical",
        "card-size": "small"
    },
    "components": {
        "title": "title",
        "subtitle": "subtitle",
        "art": {
            "field": "art",
            "fill-mode": "fit"
        },
        "attributes": "attributes"
    }
})";

const static string VIDEO_RESULT = R"({
    "schema-version": 1,
    "template": {
        "category-layout": "grid",
        "card-layout": "horizontal",
        "card-size": "small"
    },
    "components": {
        "title": "title",
        "subtitle": "subtitle",
        "art": {
            "field": "art",
            "fill-mode": "fit"
        },
        "attributes": "attributes"
    }
})";

const static string AGG_SCOPE = R"({
    "schema-version": 1,
    "template": {
        "category-layout": "grid",
        "card-layout": "vertical",
        "card-size": "medium"
    },
    "components": {
        "title": "title",
        "subtitle": "subtitle",
        "art": {
            "field": "art",
            "fill-mode": "fit",
            "aspect-ratio": 1.5
        },
        "attributes": "attributes"
    }
})";

Query::Query(const sc::CannedQuery &query,
             const sc::SearchMetadata &metadata,
             const std::string scopePath,
             Client::Config::Ptr config) :
    sc::SearchQueryBase(query, metadata),
    client_(config),
    metadata_(metadata) {
    this->scopePath = scopePath;
}

void Query::cancelled() {
    client_.cancel();
}


void Query::run(sc::SearchReplyProxy const& reply) {
    initScope();

    try {

        /****************************************************
         * 如果是聚合SCOPE
         * 1.对不同的关键字聚合SCOPE显示不同信息
         * 2.对搜索做处理
         ***************************************************/
        if (metadata_.is_aggregated()) {

            qDebug() << "[聚合 Scope]";
            auto keywords = metadata_.aggregated_keywords();

            // 聚合 SCOPE 展示
            showForAggScope(keywords, reply);

            return;
        }



        // department_id to category for search
        QMap<QString, QString> deptIdMap;

        // create departments
        sc::Department::SPtr all_depts = sc::Department::create("", query(), _("首页"));
        sc::Department::SPtr video_depts = sc::Department::create("video", query(), _("视频"));
        sc::Department::SPtr show_depts = sc::Department::create("show", query(), _("节目"));
        // sc::Department::SPtr playlist_depts = sc::Department::create("playlist", query(), "专辑");

        for (const Client::Category &c : this->getCategorise("video")) {
            video_depts->add_subdepartment(sc::Department::create("video_" + c.term, query(), c.label));
            deptIdMap[QString::fromStdString("video_" + c.term)] = QString::fromStdString(c.label);
        }
        for (const Client::Category &c : this->getCategorise("show")) {
            show_depts->add_subdepartment(sc::Department::create("show_" + c.term, query(), c.label));
            deptIdMap[QString::fromStdString("show_" + c.term)] = QString::fromStdString(c.label);
        }
        // for (const Client::Category &c : this->getCategorise("playlist")) {
        //     playlist_depts->add_subdepartment(sc::Department::create("playlist_" + c.term, query(), c.label));
        //     deptIdMap[QString::fromStdString("playlist_" + c.term)] = QString::fromStdString(c.label);
        // }
        // all_depts->set_subdepartments({video_depts, show_depts, playlist_depts});
        all_depts->set_subdepartments({video_depts, show_depts});
        reply->register_departments(all_depts);

        // qDebug() << deptIdMap;


        // Start by getting information about the query
        const sc::CannedQuery &query(sc::SearchQueryBase::query());

        // Trim the query string of whitespace
        string query_string = query.query_string();

        qDebug() << "[搜索]";
        qDebug() << "输入: " << QString::fromStdString(query_string);
        qDebug() << "分类: " << QString::fromStdString(query.department_id());

        // QString qQuery_string = QString::fromStdString(query_string);
        QString qDepartment_id = QString::fromStdString(query.department_id());


        /***************************
         * 搜索逻辑
         ***************************/
        if (query_string.empty()) {
            // 搜索词为空
            if (query.department_id().empty()) {
                // 分类为空
                // 展示预设首页
                showIndex(reply);
            } else {
                // 分类不为空，搜索词为空
                string category = deptIdMap[qDepartment_id].toStdString();
                if (qDepartment_id.startsWith("video")) {
                    // 分类为视频
                    Client::VideoList videoList = client_.getVideosByCategory(category);
                    showVideos(reply, videoList);
                } else if (qDepartment_id.startsWith("show")) {
                    // 分类为节目
                    string _category = "综艺";    // 节目需要指定分类
                    if (!category.empty()) {
                        _category = category;
                    }
                    Client::ShowList showList = client_.getShowsByCategory(_category);
                    showShows(reply, showList);
                }
            }
        } else {
            // 搜索词不为空
            if (query.department_id().empty()) {
                // 分类为空
                Client::VideoList videoList = client_.getVideosByCategory("", query_string, "history", s_orderby, std::to_string(s_resultcount));
                //showVideos(reply, videoList);
                showQueryVideos(reply, videoList);
            } else {
                // 分类不为空
                string category = deptIdMap[qDepartment_id].toStdString();
                if (qDepartment_id.startsWith("video")) {
                    Client::VideoList videoList = client_.getVideosByCategory(category, query_string, "history", s_orderby, std::to_string(s_resultcount));
                    //showVideos(reply, videoList);
                    showQueryVideos(reply, videoList);
                } else if (qDepartment_id.startsWith("show")) {
                    string _category = "综艺";    // 节目需要指定分类
                    if (!category.empty()) {
                        _category = category;
                    }
                    Client::ShowList showList = client_.getShowsByCategory(_category, query_string, s_orderby, std::to_string(s_resultcount));
                    //showShows(reply, showList);
                    showQueryShows(reply, showList);
                }
            }
        }
        /****************************/

    } catch (domain_error &e) {
        // Handle exceptions being thrown by the client API
        cerr << e.what() << endl;
        reply->error(current_exception());
    }
}

void Query::initScope() {
    unity::scopes::VariantMap config = settings();  // The settings method is provided by the base class
    if (config.empty())
        cerr << "CONFIG EMPTY!" << endl;

    qDebug() << "Init...";

    // 首页TOP分类
    int indexValue = config["indextop"].get_int();
    s_indextop = indexTopShowCategories[indexValue];
    // 搜索结果数
    s_resultcount = config["resultcount"].get_double();
    // 搜索排序
    int orderbyValue = config["orderby"].get_int();
    s_orderby = resultOrderby[orderbyValue];

    cerr << "indexTop: " << s_indextop << endl;
    cerr << "resultcount: " << s_resultcount << endl;
    cerr << "orderby: " << s_orderby << endl;
}

void Query::showForAggScope(const std::set<string> keywords, const unity::scopes::SearchReplyProxy &reply) {

    string category = "";   // 具体分类
    string _type = "video"; // video 或 show
    srand((unsigned)time(0));

    /******************************
     * 对聚合关键词的支持
     *****************************/
    if (keywords.find("videos") != keywords.end() || keywords.find("video") != keywords.end()) {
        /**********
         * 视频聚合
         *********/
        // 随机视频或节目的所有子分类
        int ran = rand() % 2;
        if (ran == 0) {
            // 节目
            _type = "show";
        }
        int ran2 = rand() % getCategorise(_type).size();
        category = getCategorise(_type)[ran2].label;

    } else if (keywords.find("music") != keywords.end()) {
        /***********
         * 音乐聚合
         **********/
        category = "音乐";

    } else if (keywords.find("news") != keywords.end()) {
        /***********
         * 新闻聚合
         **********/
        QList<string> categorys = {"资讯", "娱乐", "体育资讯", "游戏资讯"};
        int ran2 = rand() % categorys.size();
        category = categorys[ran2];

    } else if (keywords.find("gaming") != keywords.end()) {
        /***********
         * 游戏聚合
         **********/
        category = "游戏";

    } else if (keywords.find("kids") != keywords.end()) {
        /***********
         * 儿童聚合
         **********/
        QList<string> categorys = {"动漫", "亲子"};
        int ran2 = rand() % categorys.size();
        category = categorys[ran2];

    } else if (keywords.find("educational") != keywords.end()) {
        /***********
         * 教育聚合
         **********/
        int ran = rand() % 2;
        if (ran == 0) {
            // 节目
            _type = "show";
        }
        category = "教育";

    } else if (keywords.find("finance") != keywords.end()) {
        /***********
         * 财经聚合
         **********/
        category = "财经资讯";

    } else if (keywords.find("humor") != keywords.end()) {
        /***********
         * 幽默聚合
         **********/
        category = "搞笑";

    } else if (keywords.find("lifestyle") != keywords.end()) {
        /***********
         * 生活聚合
         **********/
        category = "生活";

    } else if (keywords.find("movies") != keywords.end()) {
        /***********
         * 电影聚合
         **********/
        int ran = rand() % 2;
        if (ran == 0) {
            // 节目
            _type = "show";
        }
        QList<string> categorys = {"电影", "微电影"};
        category = "电影";
        if (_type == "video") {
            int ran2 = rand() % categorys.size();
            category = categorys[ran2];
        }

    } else if (keywords.find("science") != keywords.end()) {
        /***********
         * 科技聚合
         **********/
        category = "科技";

    } else if (keywords.find("shopping") != keywords.end()) {
        /***********
         * 购物聚合
         **********/
        QList<string> categorys = {"时尚", "广告"};
        int ran2 = rand() % categorys.size();
        category = categorys[ran2];

    } else if (keywords.find("sports") != keywords.end()) {
        /***********
         * 体育聚合
         **********/
        int ran = rand() % 2;
        if (ran == 0) {
            // 节目
            _type = "show";
        }
        category = "体育";

    } else if (keywords.find("travel") != keywords.end()) {
        /***********
         * 旅游聚合
         **********/
        category = "旅游";

    } else if (keywords.find("tv") != keywords.end()) {
        /***********
         * 电视聚合
         **********/
        int ran = rand() % 2;
        if (ran == 0) {
            // 节目
            _type = "show";
        }
        QList<string> categorys = {"电视剧", "网剧", "综艺", "纪录片"};

        int ran2 = rand() % categorys.size();
        category = categorys[ran2];

    } else if (keywords.find("comics") != keywords.end()) {
        /***********
         * 漫画聚合
         **********/
        category = "动漫";

    } else {
        // 不支持的聚合
        return;
    }

    qDebug() << QString::fromStdString(_type);
    qDebug() << QString::fromStdString(category);

    // 检查是否有搜索
    const sc::CannedQuery &query(sc::SearchQueryBase::query());
    string query_string = query.query_string();
    if (!query_string.empty()) {

        qDebug() << "[聚合搜索]: " << QString::fromStdString(query_string);
        qDebug() << QString::fromStdString(_type);
        qDebug() << QString::fromStdString(category);

        Client::VideoList videoList = client_.getVideosByCategory("", query_string, "history", s_orderby, std::to_string(s_resultcount));
        showQueryVideos(reply, videoList);
        return;
    }

    if (_type == "video") {
        // 需要展示视频
        showAggVideos(category, reply);

    } else if (_type == "show") {
        // 需要展示节目
        showAggShows(category, reply);
    }

}

void Query::showAggVideos(const string &category, const unity::scopes::SearchReplyProxy &reply) {

    sc::CategoryRenderer rdrGrid(AGG_SCOPE);
    Client::VideoList videoList = client_.getVideosByCategory(category);
    auto video_cat = reply->register_category("agg_video_grid", "今日" + category + "节目推荐", this->getIconPath("icon"), rdrGrid);
    for (const auto &video : videoList) {

        const std::shared_ptr<const sc::Category> * category;
        category = &video_cat;
        sc::CategorisedResult res((*category));

        res["type"] = "video";
        res["_id"] = video.id;
        res.set_uri(video.link);
        res.set_title(video.title);
        res.set_art(video.thumbnail);

        unity::scopes::VariantArray attributes;

        unity::scopes::VariantMap countAttribute;
        countAttribute["icon"] = this->getIconPath("view");
        countAttribute["value"] = this->formatCount(video.view_count);
        attributes.emplace_back(countAttribute);

        unity::scopes::VariantMap upAttribute;
        upAttribute["icon"] = this->getIconPath("up");
        upAttribute["value"] = this->formatCount(video.up_count);
        attributes.emplace_back(upAttribute);

        unity::scopes::VariantMap downAttribute;
        downAttribute["icon"] = this->getIconPath("down");
        downAttribute["value"] = this->formatCount(video.down_count);
        attributes.emplace_back(downAttribute);

        res["attributes"] = attributes;

        if (!reply->push(res)) {
            return;
        }
    }
}

void Query::showAggShows(const string &category, const unity::scopes::SearchReplyProxy &reply) {

    sc::CategoryRenderer rdrGrid(AGG_SCOPE);
    Client::ShowList showList = client_.getShowsByCategory(category);
    auto show_cat = reply->register_category("agg_show_grid", "今日" + category + "节目推荐", this->getIconPath("icon"), rdrGrid);
    for (const auto &show : showList) {
        const std::shared_ptr<const sc::Category> * category;
        category = &show_cat;
        sc::CategorisedResult res((*category));

        res["type"] = "show";
        res["_id"] = show.id;
        res.set_uri(show.link);
        res.set_title(show.name);
        res.set_art(show.thumbnail);

        unity::scopes::VariantArray attributes;

        unity::scopes::VariantMap countAttribute;
        countAttribute["icon"] = this->getIconPath("view");
        countAttribute["value"] = this->formatCount(show.view_count);
        attributes.emplace_back(countAttribute);

        res["subtitle"] = "更新至" + show.episode_updated;
        res["attributes"] = attributes;

        if (!reply->push(res)) {
            break;
        }
    }
}


void Query::showVideos(const unity::scopes::SearchReplyProxy &reply, const Client::VideoList &videoList) {

    qDebug() << "---------";
    qDebug() << "SHOW VIDEOS";
    qDebug() << "---------";

    sc::CategoryRenderer rdrGrid(VIDEO_GRID);
    sc::CategoryRenderer rdrCarousel(VIDEO_CAROUSEL);

    // Register a category for the current weather, with the title we just built
    auto video_cat_carousel = reply->register_category("video_carousel", "今日播放TOP10", "", rdrCarousel);
    auto video_cat_grid = reply->register_category("video_grid", "", "", rdrGrid);

    int count = 0;

    for (const auto &video : videoList) {

        const std::shared_ptr<const sc::Category> * category;
        count ++;
        if (count <= 10) {
            category = &video_cat_carousel;
        } else {
            category = &video_cat_grid;
        }
        sc::CategorisedResult res((*category));

        res["type"] = "video";
        res["_id"] = video.id;
        res.set_uri(video.link);
        res.set_title(video.title);
        res.set_art(video.thumbnail);

        // template attributes
        unity::scopes::VariantArray attributes;

        unity::scopes::VariantMap countAttribute;
        //countAttribute["icon"] = this->getIconPath("play");
        countAttribute["value"] = "📺 " + this->formatCount(video.view_count);
        attributes.emplace_back(countAttribute);

        res["attributes"] = attributes;

        if (!reply->push(res)) {
            return;
        }
    }

}

void Query::showShows(const unity::scopes::SearchReplyProxy &reply, const Client::ShowList &showList) {

    qDebug() << "---------";
    qDebug() << "SHOW SHOWS";
    qDebug() << "---------";

    sc::CategoryRenderer rdrGrid(VIDEO_GRID);
    sc::CategoryRenderer rdrCarousel(VIDEO_CAROUSEL);

    // Register a category for the current weather, with the title we just built
    auto show_cat_carousel = reply->register_category("show_carousel", "今日播放TOP10", "", rdrCarousel);
    auto show_cat_grid = reply->register_category("show_grid", "", "", rdrGrid);

    int count = 0;

    for (const auto &show : showList) {

        const std::shared_ptr<const sc::Category> * category;
        count ++;
        if (count <= 10) {
            category = &show_cat_carousel;
        } else {
            category = &show_cat_grid;
        }
        sc::CategorisedResult res((*category));

        res["type"] = "show";
        res["_id"] = show.id;
        res.set_uri(show.link);
        res.set_title(show.name);
        res.set_art(show.thumbnail);

        // template attributes
        unity::scopes::VariantArray attributes;

        unity::scopes::VariantMap countAttribute;
        //countAttribute["icon"] = this->getIconPath("play");
        countAttribute["value"] = "📺 " + this->formatCount(show.view_count);
        attributes.emplace_back(countAttribute);

        res["attributes"] = attributes;

        if (!reply->push(res)) {
            return;
        }
    }

}

/********************************************************
 * Video Query Result
 * ******************************************************/
void Query::showQueryVideos(const unity::scopes::SearchReplyProxy &reply, const Client::VideoList &videoList) {

    sc::CategoryRenderer rdrGrid(VIDEO_RESULT);

    auto video_cat_grid = reply->register_category("video_query_result", _("搜索结果"), "", rdrGrid);

    for (const auto &video : videoList) {

        const std::shared_ptr<const sc::Category> * category;

        category = &video_cat_grid;

        sc::CategorisedResult res((*category));

        res["type"] = "video";
        res["_id"] = video.id;
        res.set_uri(video.link);
        res.set_title(video.title);
        res.set_art(video.thumbnail);
        res["subtitle"] = video.published;

        // template attributes
        unity::scopes::VariantArray attributes;

        unity::scopes::VariantMap countAttribute;
        //countAttribute["icon"] = this->getIconPath("play");
        countAttribute["value"] = "📺 " + this->formatCount(video.view_count);
        attributes.emplace_back(countAttribute);

        res["attributes"] = attributes;

        if (!reply->push(res)) {
            return;
        }
    }
}

/********************************************************
 * Show Query Result
 * ******************************************************/
void Query::showQueryShows(const unity::scopes::SearchReplyProxy &reply, const Client::ShowList &showList) {

    sc::CategoryRenderer rdrGrid(VIDEO_RESULT);

    auto show_cat_grid = reply->register_category("show_query_result", _("搜索结果"), "", rdrGrid);

    for (const auto &show : showList) {

        const std::shared_ptr<const sc::Category> * category;

        category = &show_cat_grid;

        sc::CategorisedResult res((*category));

        res["type"] = "show";
        res["_id"] = show.id;
        res.set_uri(show.link);
        res.set_title(show.name);
        res.set_art(show.thumbnail);
        res["subtitle"] = show.published;

        // template attributes
        unity::scopes::VariantArray attributes;

        unity::scopes::VariantMap countAttribute;
        //countAttribute["icon"] = this->getIconPath("play");
        countAttribute["value"] = "📺 " + this->formatCount(show.view_count);
        attributes.emplace_back(countAttribute);

        res["attributes"] = attributes;

        if (!reply->push(res)) {
            return;
        }
    }
}


/********************************************************
 *  Scope Index
 * ******************************************************/
void Query::showIndex(const unity::scopes::SearchReplyProxy &reply) {

    qDebug() << "[首页设置]:" << QString::fromStdString(s_indextop);

    // get user set indextop for WeekTOP10
    // and get show list
    Client::ShowList showList = client_.getShowsByCategory(s_indextop, "", "week", "10");
    sc::CategoryRenderer rdrCarousel(VIDEO_CAROUSEL);
    auto show_cat_carousel = reply->register_category("show_carousel", "本周" + s_indextop + "节目TOP10", "", rdrCarousel);
    for (const auto &show : showList) {

        const std::shared_ptr<const sc::Category> * category;
        category = &show_cat_carousel;
        sc::CategorisedResult res((*category));

        // template basic info
        res["type"] = "show";
        res["_id"] = show.id;
        res.set_uri(show.link);
        res.set_title(show.name);
        res.set_art(show.thumbnail);
        res["source_log"] = this->getIconPath("logo");

        // template attributes
        unity::scopes::VariantArray attributes;

        unity::scopes::VariantMap scoreAttribute;
        //scoreAttribute["icon"] = this->getIconPath("up");
        //scoreAttribute["value"] = show.score;
        scoreAttribute["value"] = "💜 " + show.score;
        attributes.emplace_back(scoreAttribute);

        unity::scopes::VariantMap countAttribute;
        //countAttribute["icon"] = this->getIconPath("play");
        countAttribute["value"] = "📺 " + this->formatCount(show.view_count);
        attributes.emplace_back(countAttribute);

        res["attributes"] = attributes;

        if (!reply->push(res)) {
            //return;
            break;
        }
    }

    // ==========================
    // section 2
    // ==========================
    string rand_category;
    srand((unsigned)time(0));
    int ran = rand() % this->getCategorise("show").size();
    rand_category = getCategorise("show")[ran].label;

    showList = client_.getShowsByCategory(rand_category, "", "today", "24");
    sc::CategoryRenderer rdrGrid(VIDEO_GRID);
    show_cat_carousel = reply->register_category("section2", "今日" + rand_category + "节目推荐", "", rdrGrid);

    for (const auto &show : showList) {

        const std::shared_ptr<const sc::Category> * category;
        category = &show_cat_carousel;
        sc::CategorisedResult res((*category));

        res["type"] = "show";
        res["_id"] = show.id;
        res.set_uri(show.link);
        res.set_title(show.name);
        res.set_art(show.thumbnail);

        unity::scopes::VariantArray attributes;

        unity::scopes::VariantMap countAttribute;
        countAttribute["icon"] = this->getIconPath("view");
        countAttribute["value"] = this->formatCount(show.view_count);
        attributes.emplace_back(countAttribute);

        res["subtitle"] = "更新至" + show.episode_updated;
        res["attributes"] = attributes;

        if (!reply->push(res)) {
            break;
        }
    }

    // ==========================
    // section 3
    // ==========================

    ran = rand() % this->getCategorise("video").size();
    rand_category = getCategorise("video")[ran].label;

    Client::VideoList videoList = client_.getVideosByCategory(rand_category, "", "today", "24");
    auto video_cat_carousel = reply->register_category("section3", "今日" + rand_category + "视频推荐", "", rdrGrid);

    for (const auto &video : videoList) {

        const std::shared_ptr<const sc::Category> * category;
        category = &video_cat_carousel;
        sc::CategorisedResult res((*category));

        res["type"] = "video";
        res["_id"] = video.id;
        res.set_uri(video.link);
        res.set_title(video.title);
        res.set_art(video.thumbnail);

        unity::scopes::VariantArray attributes;

        unity::scopes::VariantMap countAttribute;
        countAttribute["icon"] = this->getIconPath("view");
        countAttribute["value"] = this->formatCount(video.view_count);
        attributes.emplace_back(countAttribute);

        unity::scopes::VariantMap upAttribute;
        upAttribute["icon"] = this->getIconPath("up");
        upAttribute["value"] = this->formatCount(video.up_count);
        attributes.emplace_back(upAttribute);

        unity::scopes::VariantMap downAttribute;
        downAttribute["icon"] = this->getIconPath("down");
        downAttribute["value"] = this->formatCount(video.down_count);
        attributes.emplace_back(downAttribute);

        res["attributes"] = attributes;

        if (!reply->push(res)) {
            return;
        }
    }
}

std::string Query::getIconPath(std::string source) {
    std::string basePath = this->scopePath;
    std::string path;

    path = basePath + "/" + source + ".png";

    return path;
}

std::string Query::formatCount(std::string count) {
    int s_len = count.length();
    // view count may very long
    unsigned long long _count = std::stoull(count);

    if (s_len <= 3) {
        return count;
    }
    if (s_len > 8) {
        return to_string(_count / 100000000) + "亿";
    } else if (s_len > 4) {
        return to_string(_count / 10000) + "万";
    } else if (s_len > 3) {
        return to_string(_count / 1000) + "千";
    }
    return count;
}

Client::CategoryList Query::getCategorise(const string &type) {

    qDebug() << QString::fromStdString(this->scopePath) << "***";
    QFile file(QString::fromStdString(this->scopePath + "/category.json"));
    file.open(QIODevice::ReadOnly | QIODevice::Text);
    QJsonDocument root = QJsonDocument::fromJson(file.readAll());
    file.close();


    Client::CategoryList categoryList;
    QVariantMap variant = root.toVariant().toMap();
    QString _type = QString::fromStdString(type);

    for (const QVariant &i : variant[_type].toList()) {
        QVariantMap item = i.toMap();

        categoryList.emplace_back(
            Client::Category {
                item["term"].toString().toStdString(),
                item["label"].toString().toStdString(),
                item["lang"].toString().toStdString()
            }
        );
    }

    return categoryList;

}

