#ifndef QUERY_H_
#define QUERY_H_

#include <client.h>
#include <scope.h>

#include <unity/scopes/SearchQueryBase.h>
#include <unity/scopes/ReplyProxyFwd.h>

/**
 * Represents an individual query.
 *
 * A new Query object will be constructed for each query. It is
 * given query information, metadata about the search, and
 * some scope-specific configuration.
 */
class Query: public unity::scopes::SearchQueryBase {
public:
    Query(const unity::scopes::CannedQuery &query,
          const unity::scopes::SearchMetadata &metadata,
          const std::string scopePath,
          Client::Config::Ptr config);

    ~Query() = default;

    void cancelled() override;

    void run(const unity::scopes::SearchReplyProxy &reply) override;

private:
    Client client_;
    std::string scopePath;
    unity::scopes::SearchMetadata const metadata_;
    std::string s_indextop;
    int s_resultcount;
    std::string s_orderby;

    void initScope();

    void showForAggScope(const std::set<std::string> keywords, const unity::scopes::SearchReplyProxy &reply);

    void showAggVideos(const std::string &category, const unity::scopes::SearchReplyProxy &reply);

    void showAggShows(const std::string &category, const unity::scopes::SearchReplyProxy &reply);

    void showVideos(const unity::scopes::SearchReplyProxy &reply, const Client::VideoList &videoList);

    void showShows(const unity::scopes::SearchReplyProxy &reply, const Client::ShowList &showList);

    void showIndex(const unity::scopes::SearchReplyProxy &reply);

    void showQueryVideos(const unity::scopes::SearchReplyProxy &reply, const Client::VideoList &videoList);

    void showQueryShows(const unity::scopes::SearchReplyProxy &reply, const Client::ShowList &showList);


    std::string getIconPath(std::string source);
    std::string formatCount(std::string count);
    Client::CategoryList getCategorise(const std::string &type);
};

#endif // QUERY_H_


