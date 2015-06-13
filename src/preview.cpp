#include <preview.h>

#include <unity/scopes/ColumnLayout.h>
#include <unity/scopes/PreviewWidget.h>
#include <unity/scopes/PreviewReply.h>
#include <unity/scopes/Result.h>
#include <unity/scopes/VariantBuilder.h>

#include <iostream>

namespace sc = unity::scopes;

using namespace std;

Preview::Preview(const sc::Result &result, const sc::ActionMetadata &metadata, Client::Config::Ptr config) :
    sc::PreviewQueryBase(result, metadata), client_(config) {
}

void Preview::cancelled() {
}

void Preview::run(sc::PreviewReplyProxy const& reply) {

    sc::Result result = PreviewQueryBase::result();

    // 逻辑判断，按类别做不同展示
    string showType = result["type"].get_string();
    if (showType == "video") {
            this->runVideo(reply);
    } else if (showType == "show") {
            this->runShow(reply);
            //this->runTest(reply);
    }

}


// 展示Video
void Preview::runVideo(const unity::scopes::PreviewReplyProxy &reply) {

    sc::Result result = PreviewQueryBase::result();

    std::string video_id = result["_id"].get_string();
    Client::VideoDetail vDetail = client_.getVideoDetailById(video_id);

    // 列布局
    sc::ColumnLayout layout1col(1), layout2col(2), layout3col(3);

    layout1col.add_column({"wvideo", "wheader", "wattrs", "wdescription", "wactions"});
    layout2col.add_column({"wvideo", "wheader", "wactions"});
    layout2col.add_column({"wattrs", "wdescription"});
    layout3col.add_column({"wvideo", "wheader", "wactions"});
    layout3col.add_column({"wattrs"});
    layout3col.add_column({"wdescription"});

    // 注册布局
    reply->register_layout({layout1col});

    // 视频控件
    sc::PreviewWidget widget_video("wvideo", "video");
    if (vDetail.bigThumbnail.length() > 0) {
        widget_video.add_attribute_value("screenshot", sc::Variant(vDetail.bigThumbnail));
    } else {
        widget_video.add_attribute_value("screenshot", sc::Variant(vDetail.thumbnail));
    }
    widget_video.add_attribute_value("source", sc::Variant(vDetail.link));
    // 标题头控件
    sc::PreviewWidget widget_header("wheader", "header");
    widget_header.add_attribute_value("title", sc::Variant(result.title()));
    int sec = vDetail.duration;
    if (sec <= 60) {
        widget_header.add_attribute_value("subtitle", sc::Variant("时长: " + std::to_string(vDetail.duration) + " 秒"));
    } else {
        widget_header.add_attribute_value("subtitle", sc::Variant("时长: " + std::to_string(vDetail.duration / 60) + " 分钟"));
    }
    // 属性
    sc::PreviewWidget widget_attrs("wattrs", "text");
    widget_attrs.add_attribute_value("title", sc::Variant("信息: "));
    widget_attrs.add_attribute_value(
        "text",
        sc::Variant("类型: " + vDetail.category
                    + "\n标签: " + vDetail.tags
                    + "\n发布时间: " + vDetail.published
                    + "\n总播放: " + std::to_string(vDetail.view_count)
                    + "\n评论: " + std::to_string(vDetail.comment_count)
                    + " / 收藏: " + std::to_string(vDetail.favorite_count)
                    + "\n顶: " + std::to_string(vDetail.up_count)
                    + " / 踩: " + std::to_string(vDetail.down_count)
        )
    );
    // 描述
    sc::PreviewWidget widget_description("wdescription", "text");
    widget_description.add_attribute_value("title", sc::Variant("描述:"));
    if (vDetail.description.length() > 0) {
        widget_description.add_attribute_value("text", sc::Variant(vDetail.description));
    } else {
        widget_description.add_attribute_value("text", sc::Variant("无"));
    }

    // 按钮
    sc::PreviewWidget widget_actions("wactions", "actions");
    sc::VariantBuilder builder;
    builder.add_tuple({
        {"id", sc::Variant("website")},
        {"label", sc::Variant("开始播放")},
        {"uri", sc::Variant(vDetail.link)}
    });
//    builder.add_tuple({
//        {"id", sc::Variant("videocomment")},
//        {"label", sc::Variant("查看评论")},
//        {"uri", sc::Variant(vDetail.link + "#videocomment")}
//    });
    widget_actions.add_attribute_value("actions", builder.end());

    reply->push({widget_video, widget_header, widget_attrs, widget_description, widget_actions});
}

// 展示Show
void Preview::runShow(const unity::scopes::PreviewReplyProxy &reply) {

    sc::Result result = PreviewQueryBase::result();

    std::string show_id = result["_id"].get_string();
    Client::ShowDetail sDetail = client_.getShowDetailById(show_id);

    // 列布局
    sc::ColumnLayout layout1col(1), layout2col(2), layout3col(3);

    layout1col.add_column({"wvideo", "wheader", "wattrs", "wdescription", "wacts"});
    layout2col.add_column({"wvideo", "wheader", "wacts"});
    layout2col.add_column({"wattrs", "wdescription"});
    layout3col.add_column({"wvideo", "wheader", "wacts"});
    layout3col.add_column({"wattrs"});
    layout3col.add_column({"wdescription"});

    // 注册布局
    reply->register_layout({layout1col, layout2col});

    // 视频控件
    sc::PreviewWidget widget_video("wvideo", "video");
    // widget_video.add_attribute_value("screenshot", sc::Variant(result.art()));
    widget_video.add_attribute_value("screenshot", sc::Variant(sDetail.thumbnail));
    widget_video.add_attribute_value("source", sc::Variant(sDetail.play_link));

    // 标题头控件
    sc::PreviewWidget widget_header("wheader", "header");
    widget_header.add_attribute_value("title", sc::Variant(result.title()));
    widget_header.add_attribute_value("subtitle", sc::Variant("评分: " + std::to_string(sDetail.score)));

    // 文本控件
    // 属性
    sc::PreviewWidget widget_attrs("wattrs", "text");
    widget_attrs.add_attribute_value("title", sc::Variant("信息: "));
    widget_attrs.add_attribute_value(
        "text",
        sc::Variant("类型: " + sDetail.genre
                     + "\n地区: " + sDetail.area
                     + "\n上映: " + sDetail.released
                     + "\n总集数: " + std::to_string(sDetail.episode_count)
                     + " / 更新至: " + std::to_string(sDetail.episode_updated)
                     + "\n总播放: " + std::to_string(sDetail.view_count)
                     + " / 昨日播放: " + std::to_string(sDetail.view_yesterday_count)
                     + "\n评论: " + std::to_string(sDetail.comment_count)
                     + " / 收藏: " + std::to_string(sDetail.favorite_count)
                     + "\n顶: " + std::to_string(sDetail.up_count)
                     + " / 踩: " + std::to_string(sDetail.down_count)
        )
    );
    // 描述
    sc::PreviewWidget widget_description("wdescription", "text");
    widget_description.add_attribute_value("title", sc::Variant("描述:"));
    if (sDetail.description.length() > 0) {
        widget_description.add_attribute_value("text", sc::Variant(sDetail.description));
    } else {
        widget_description.add_attribute_value("text", sc::Variant("无"));
    }

    // 按钮
    sc::PreviewWidget widget_actions("wacts", "actions");
    sc::VariantBuilder builder;
    builder.add_tuple({
        {"id", sc::Variant("website")},
        {"label", sc::Variant("分集播放")},
        {"uri", sc::Variant(sDetail.link)}
    });
    widget_actions.add_attribute_value("actions", builder.end());

    reply->push({widget_video, widget_header, widget_attrs, widget_description, widget_actions});
}


// 展示TEST
void Preview::runTest(const unity::scopes::PreviewReplyProxy &reply) {
    sc::Result result = PreviewQueryBase::result();

    // 列布局
    sc::ColumnLayout layout1col(1);

    layout1col.add_column({"wgallery", "wvideo", "wheader"});

    // 注册布局
    reply->register_layout({layout1col});

    // 视频控件
    sc::PreviewWidget widget_video("wvideo", "video");
    widget_video.add_attribute_value("screenshot", sc::Variant(result.art()));
    widget_video.add_attribute_value("source", sc::Variant(result.uri()));
    // Header控件
    sc::PreviewWidget widget_header("wheader", "header");
    widget_header.add_attribute_value("title", sc::Variant(result.title()));
    widget_header.add_attribute_mapping("subtitle", "subtitle");

    sc::PreviewWidget widget_gallery("wgallery", "gallery");
    sc::VariantArray arr;
    arr.push_back(sc::Variant(result.art()));
    arr.push_back(sc::Variant(result.art()));
    arr.push_back(sc::Variant(result.art()));
    arr.push_back(sc::Variant(result.art()));
    arr.push_back(sc::Variant(result.art()));
    widget_gallery.add_attribute_value("sources", sc::Variant(arr));

    reply->push({widget_video, widget_header, widget_gallery});
}

