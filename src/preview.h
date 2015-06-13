#ifndef PREVIEW_H_
#define PREVIEW_H_

#include <unity/scopes/PreviewQueryBase.h>
#include <client.h>

namespace unity {
namespace scopes {
class Result;
}
}

/**
 * Represents an individual preview request.
 *
 * Each time a result is previewed in the UI a new Preview
 * object is created.
 */
class Preview: public unity::scopes::PreviewQueryBase {

private:
    Client client_;

public:
    Preview(const unity::scopes::Result &result,
            const unity::scopes::ActionMetadata &metadata,
            Client::Config::Ptr config);

    ~Preview() = default;

    void cancelled() override;

    /**
     * Populates the reply object with preview information.
     */
    void run(unity::scopes::PreviewReplyProxy const& reply) override;

    /**
     * 展示Video信息
     */
    void runVideo(unity::scopes::PreviewReplyProxy const& reply);

    /**
     * 展示Show信息
     */
    void runShow(unity::scopes::PreviewReplyProxy const& reply);


    void runTest(unity::scopes::PreviewReplyProxy const& reply);
};

#endif // PREVIEW_H_

