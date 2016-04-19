package main

import (
	"fmt"
	"launchpad.net/go-onlineaccounts/v1"
	"launchpad.net/go-unityscopes/v2"
	"log"
	"math/rand"
	"os"
	"strconv"
	"strings"
	"time"
)

const (
	packageName        = "youku.ubuntu-dawndiy"
	scopeName          = packageName + "_youku"
	accountService     = scopeName
	accountServiceType = "sharing"
	accountProvider    = packageName + "_account-plugin"
)

const (
	homeCategoryTemplate = `{
		"schema-version": 1,
		"template": {
			"category-layout": "carousel",
			"card-size": "large",
			"overlay": true
		},
		"components": {
			"title": "title",
			"subtitle": "subtitle",
			"art": {
				"field": "art",
				"aspect-ratio": 1.4
			},
			"attributes": "attributes"
		}
	}`

	custormVideoCategoryTemplate = `{
		"schema-verion": 1,
		"template": {
			"category-layout": "grid",
			"card-size": "%s"
		},
		"components": {
			"title": "title",
			"subtitle": "subtitle",
			"art": {
				"field": "art",
				"aspect-ratio": 1.5
			},
			"attributes": "attributes"
		}
	}`

	largeVideoCategoryTemplate = `{
		"schema-verion": 1,
		"template": {
			"category-layout": "vertical-journal",
			"card-size": "large",
			"overlay": true
		},
		"components": {
			"title": "title",
			"subtitle": "subtitle",
			"art": {
				"field": "art",
				"aspect-ratio": 2.5
			},
			"attributes": "attributes"
		}
	}`

	loginNagTemplate = `{
		"schema-version": 1,
		"template": {
			"category-layout": "vertical-journal",
			"card-size": "large",
			"card-background": "color:///#06A7E1"
		},
		"components": {
			"title": "title",
			"art" : {
				"aspect-ratio": 100.0
			}
		}
	}`

	queryVideoTemplate = `{
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
				"fill-mode": "fit",
				"aspect-ratio": 1.5
			},
			"attributes": "attributes"
		}
	}`
)

var itemSize = "medium"
var logger = log.New(os.Stdout, "", log.LstdFlags|log.Lshortfile)

type settings struct {
	ResultCount  float64 `json:"result_count"`
	ItemSize     int     `json:"item_size"`
	CommentCount float64 `json:"comment_count"`
}

// YoukuScope for Ubuntu Touch
type YoukuScope struct {
	Accounts      *accounts.Watcher
	base          *scopes.ScopeBase
	ScopeSettings *settings
}

// SetScopeBase to set the ScopeBase including settings and various directories available for use
func (sc *YoukuScope) SetScopeBase(base *scopes.ScopeBase) {
	sc.base = base
}

// Search to display items
func (sc *YoukuScope) Search(query *scopes.CannedQuery, metadata *scopes.SearchMetadata, reply *scopes.SearchReply, cancelled <-chan bool) error {

	// Get Settings
	var s settings
	err := sc.base.Settings(&s)
	if err != nil {
		logger.Println("[ERROR]", err)
		sc.ScopeSettings = &settings{50, 1, 20}
	} else {
		sc.ScopeSettings = &s
	}

	// Parse Settings
	switch sc.ScopeSettings.ItemSize {
	case 0:
		itemSize = "large"
	case 1:
		itemSize = "medium"
	case 2:
		itemSize = "small"
	}

	queryString := query.QueryString()
	departmentID := query.DepartmentID()

	logger.Printf("[Query] isAgg: %t \t dptID: %s \t query: %s\n", metadata.IsAggregated(), departmentID, queryString)

	// If search from Aggregated Scopes
	if metadata.IsAggregated() {
		sc.showForAggregatedScopes(query, metadata, reply)
		return nil
	}

	// Check Login
	service, err := sc.Accounts.FirstService()
	if err != nil {
		logger.Println("[ERROR] Could not account data: ", err)
	}
	if service == nil {
		cat := reply.RegisterCategory("nag", "", "", loginNagTemplate)
		result := scopes.NewCategorisedResult(cat)
		result.SetTitle("Log-in")
		scopes.RegisterAccountLoginResult(result, query, accountService, accountServiceType, accountProvider, scopes.PostLoginInvalidateResults, scopes.PostLoginDoNothing)
		if err := reply.Push(result); err != nil {
			logger.Println("[ERROR]", err)
			return err
		}
	}

	// Create departments
	reply.RegisterDepartments(sc.createDepartment(query, metadata, reply))

	if queryString == "" {
		switch {
		case strings.HasPrefix(departmentID, "home"), departmentID == "":
			sc.showHome(query, metadata, reply)
		case strings.HasPrefix(departmentID, "video"):
			sc.showVideos(query, metadata, reply)
		case strings.HasPrefix(departmentID, "show"):
			sc.showShows(query, metadata, reply)
		}
	} else {
		switch {
		case strings.HasPrefix(departmentID, "home"), departmentID == "":
			sc.queryVideo(queryString, departmentID, reply)
			sc.queryShow(queryString, departmentID, reply)
		case strings.HasPrefix(departmentID, "video"):
			sc.queryVideo(queryString, departmentID, reply)
		case strings.HasPrefix(departmentID, "show"):
			sc.queryShow(queryString, departmentID, reply)
		}
	}

	return nil
}

// Preview selected item
func (sc *YoukuScope) Preview(result *scopes.Result, metadata *scopes.ActionMetadata, reply *scopes.PreviewReply, cancelled <-chan bool) error {

	// Get Settings
	var s settings
	err := sc.base.Settings(&s)
	if err != nil {
		logger.Println("[ERROR]", err)
		sc.ScopeSettings = &settings{50, 1, 20}
	} else {
		sc.ScopeSettings = &s
	}

	var previewType string
	err = result.Get("type", &previewType)
	if err != nil {
		logger.Println("[ERROR]", err)
		return nil
	}

	logger.Println("[PREVIEW]", previewType, result.Title())

	switch previewType {
	case "video":
		sc.viewVideo(result, reply)
	case "show":
		sc.viewShow(result, reply)
	}

	return nil
}

func (sc *YoukuScope) showVideos(query *scopes.CannedQuery, metadata *scopes.SearchMetadata, reply *scopes.SearchReply) {

	// create filter
	state := query.FilterState()
	filter := scopes.NewOptionSelectorFilter("video_orderby", "Orderby", false)
	filter.DisplayHints = 1
	filter.AddOption("published", "发布时间")
	filter.AddOption("view-count", "总播放数")
	filter.AddOption("comment-count", "总评论数")
	filter.AddOption("reference-count", "总引用数")
	filter.AddOption("favorite-time", "收藏时间")
	filter.AddOption("favorite-count", "总收藏数")
	if !filter.HasActiveOption(state) {
		filter.UpdateState(state, "published", true)
	}

	filterIDs := filter.ActiveOptions(state)
	orderby := ""
	if len(filterIDs) > 0 {
		orderby = filterIDs[0]
	}
	reply.PushFilters([]scopes.Filter{filter}, state)

	_deptIDs := strings.Split(query.DepartmentID(), "_")
	var videoCategory, videoGenre string
	if len(_deptIDs) == 2 {
		videoCategory = _deptIDs[1]
	} else if len(_deptIDs) > 2 {
		videoCategory = _deptIDs[1]
		videoGenre = _deptIDs[2]
	}

	category := reply.RegisterCategory("video", videoCategory+"视频", "", fmt.Sprintf(custormVideoCategoryTemplate, itemSize))

	// Get videos
	logger.Println("[VIDEOS]", videoCategory, videoGenre, orderby)
	videos := getVideosByCategory(videoCategory, videoGenre, "today", orderby, 1, int(sc.ScopeSettings.ResultCount))

	// Show Videos
	pushData(videos, category, reply)
}

func (sc *YoukuScope) showShows(query *scopes.CannedQuery, metadata *scopes.SearchMetadata, reply *scopes.SearchReply) {

	// create filter
	state := query.FilterState()
	filter := scopes.NewOptionSelectorFilter("show_orderby", "Orderby", false)
	filter.DisplayHints = 1
	filter.AddOption("view-today-count", "今日播放数")
	filter.AddOption("view-count", "总播放数")
	filter.AddOption("comment-count", "总评论数")
	filter.AddOption("favorite-count", "总收藏数")
	filter.AddOption("view-week-count", "本周播放数")
	filter.AddOption("release-date", "上映日期")
	filter.AddOption("score", "评分")
	filter.AddOption("updated", "最后更新")
	if !filter.HasActiveOption(state) {
		filter.UpdateState(state, "view-today-count", true)
	}

	filterIDs := filter.ActiveOptions(state)
	orderby := ""
	if len(filterIDs) > 0 {
		orderby = filterIDs[0]
	}
	reply.PushFilters([]scopes.Filter{filter}, state)

	_deptIDs := strings.Split(query.DepartmentID(), "_")
	var showCategory, showGenre string
	if len(_deptIDs) == 1 {
		rand.Seed(time.Now().UnixNano())
		showCategories := getShowCategories(sc.base.ScopeDirectory())
		showCategory = showCategories[rand.Intn(len(showCategories))].Label
	} else if len(_deptIDs) == 2 {
		showCategory = _deptIDs[1]
	} else if len(_deptIDs) > 2 {
		showCategory = _deptIDs[1]
		showGenre = _deptIDs[2]
	}
	logger.Println("[SHOWS]", showCategory, showGenre, orderby)
	shows := getShowsByCategory(showCategory, showGenre, orderby, 1, int(sc.ScopeSettings.ResultCount))

	category := reply.RegisterCategory("show", showCategory+"节目", "", fmt.Sprintf(custormVideoCategoryTemplate, itemSize))

	// Show shows
	pushData(shows, category, reply)
}

func (sc *YoukuScope) showHome(query *scopes.CannedQuery, metadata *scopes.SearchMetadata, reply *scopes.SearchReply) {

	logger.Println("--SHOW HOME--")
	rand.Seed(time.Now().UnixNano())
	videoCatgories := getVideoCategories(sc.base.ScopeDirectory())
	showCategories := getShowCategories(sc.base.ScopeDirectory())

	// Top
	// ================================
	topType := rand.Intn(2)
	if topType == 0 {
		// show top videos
		videoCategory := videoCatgories[rand.Intn(len(videoCatgories))].Label
		videos := getVideosByCategory(videoCategory, "", "today", "view-count", 1, 10)
		category := reply.RegisterCategory("home", fmt.Sprintf("今日%s视频TOP10", videoCategory), "", homeCategoryTemplate)
		// show videos
		pushData(videos, category, reply)
	} else {
		// show top shows
		showCategory := showCategories[rand.Intn(len(showCategories))].Label
		for showCategory == "音乐" {
			showCategory = showCategories[rand.Intn(len(showCategories))].Label
		}
		shows := getShowsByCategory(showCategory, "", "view-today-count", 1, 10)
		category := reply.RegisterCategory("home", fmt.Sprintf("今日%s节目TOP10", showCategory), "", homeCategoryTemplate)
		// Show shows
		pushData(shows, category, reply)
	}

	// Section One
	// ================================
	if topType == 0 {
		showCategory := showCategories[rand.Intn(len(showCategories))].Label
		for showCategory == "音乐" {
			showCategory = showCategories[rand.Intn(len(showCategories))].Label
		}
		shows := getShowsByCategory(showCategory, "", "view-today-count", 1, 9)
		category := reply.RegisterCategory("section_one", fmt.Sprintf("%s节目", showCategory), "", fmt.Sprintf(custormVideoCategoryTemplate, itemSize))
		// Show shows
		pushData(shows, category, reply)
	} else {
		videoCategory := videoCatgories[rand.Intn(len(videoCatgories))].Label
		videos := getVideosByCategory(videoCategory, "", "today", "view-count", 1, 9)
		category := reply.RegisterCategory("section_one", fmt.Sprintf("%s视频", videoCategory), "", fmt.Sprintf(custormVideoCategoryTemplate, itemSize))
		// Show videos
		pushData(videos, category, reply)
	}

	// Section Two
	// ================================
	showCategory := showCategories[rand.Intn(len(showCategories))].Label
	for showCategory == "音乐" {
		showCategory = showCategories[rand.Intn(len(showCategories))].Label
	}
	shows := getShowsByCategory(showCategory, "", "view-today-count", 1, 10)
	category := reply.RegisterCategory("section_two_large", fmt.Sprintf("%s节目", showCategory), "", largeVideoCategoryTemplate)

	if len(shows) > 1 {
		showFirst := shows[0]
		result := scopes.NewCategorisedResult(category)
		result.SetTitle(showFirst.Name)
		result.SetArt(showFirst.Thumbnail)
		result.SetURI(showFirst.Link)
		result.Set("subtitle", fmt.Sprintf("更新 %s", fmt.Sprint(showFirst.EpisodeUpdated)))
		result.Set("attributes", []map[string]string{
			{"value": fmt.Sprintf("★%.2f", formatScore(showFirst.Score))},
			{"value": fmt.Sprintf("🔥%s", formatCount(showFirst.ViewCount))},
		})
		result.Set("show_id", showFirst.ID)
		result.Set("type", "show")
		if err := reply.Push(result); err != nil {
			logger.Println("[ERROR]", err)
		}

		shows = shows[1:]
	}
	category = reply.RegisterCategory("section_two", "", "", fmt.Sprintf(custormVideoCategoryTemplate, itemSize))
	// Show shows
	pushData(shows, category, reply)

	// Section Three
	// ================================
	videoCategory := videoCatgories[rand.Intn(len(videoCatgories))].Label
	videos := getVideosByCategory(videoCategory, "", "today", "view-count", 1, 10)
	category = reply.RegisterCategory("section_three_large", fmt.Sprintf("%s视频", videoCategory), "", largeVideoCategoryTemplate)

	if len(videos) > 1 {
		videoFirst := videos[0]
		result := scopes.NewCategorisedResult(category)
		result.SetTitle(videoFirst.Title)
		result.SetArt(videoFirst.Thumbnail)
		result.SetURI(videoFirst.Link)
		result.Set("attributes", []map[string]string{
			{"value": fmt.Sprintf("🕒%s", formatDuration(videoFirst.Duration))},
			{"value": fmt.Sprintf("🔥%s", formatCount(videoFirst.ViewCount))},
		})
		result.Set("video_id", videoFirst.ID)
		result.Set("type", "video")
		if err := reply.Push(result); err != nil {
			logger.Println("[ERROR]", err)
		}

		videos = videos[1:]
	}
	category = reply.RegisterCategory("section_three", "", "", fmt.Sprintf(custormVideoCategoryTemplate, itemSize))
	// Show videos
	pushData(videos, category, reply)
}

func (sc *YoukuScope) createDepartment(query *scopes.CannedQuery, metadata *scopes.SearchMetadata, reply *scopes.SearchReply) *scopes.Department {
	home, _ := scopes.NewDepartment("", query, "首页")

	videoDepartment, _ := scopes.NewDepartment("video", query, "视频")
	videoCatgories := getVideoCategories(sc.base.ScopeDirectory())
	for _, v := range videoCatgories {
		subID := "video_" + v.Label
		subDepartment, _ := scopes.NewDepartment(subID, query, v.Label)
		for _, genre := range v.Genres {
			genreID := subID + "_" + genre.Label
			genreDepartment, _ := scopes.NewDepartment(genreID, query, genre.Label)
			subDepartment.AddSubdepartment(genreDepartment)
		}
		videoDepartment.AddSubdepartment(subDepartment)
	}

	showDepartment, _ := scopes.NewDepartment("show", query, "节目")
	showCategories := getShowCategories(sc.base.ScopeDirectory())
	for _, v := range showCategories {
		subID := "show_" + v.Label
		subDepartment, _ := scopes.NewDepartment(subID, query, v.Label)
		for _, genre := range v.Genre {
			genreID := subID + "_" + genre.Label
			genreDepartment, _ := scopes.NewDepartment(genreID, query, genre.Label)
			subDepartment.AddSubdepartment(genreDepartment)
		}
		showDepartment.AddSubdepartment(subDepartment)
	}

	home.AddSubdepartment(videoDepartment)
	home.AddSubdepartment(showDepartment)

	return home
}

func (sc *YoukuScope) viewVideo(result *scopes.Result, reply *scopes.PreviewReply) {
	layoutOneCol := scopes.NewColumnLayout(1)
	layoutOneCol.AddColumn(
		"header",
		"video",
		"info",
		"expandable",
		"description",
		"actions",
		"comments",
	)
	layoutTwoCol := scopes.NewColumnLayout(2)
	layoutTwoCol.AddColumn(
		"header",
		"video",
		"expandable",
		"actions",
	)
	layoutTwoCol.AddColumn(
		"info",
		"description",
		"comments",
	)
	reply.RegisterLayout(layoutOneCol, layoutTwoCol)

	var videoID string
	err := result.Get("video_id", &videoID)
	if err != nil {
		logger.Println("[ERROR]", err)
		return
	}
	video := getVideoDetail(videoID)
	logger.Println("[VIDEO PREVIEW]", videoID, video.Title, video.Duration)

	// Header
	header := scopes.NewPreviewWidget("header", "header")
	header.AddAttributeValue("title", video.Title)
	duration := formatDuration(video.Duration)
	header.AddAttributeValue("subtitle", "时长: "+duration)

	// Video
	videoWidget := scopes.NewPreviewWidget("video", "video")
	videoWidget.AddAttributeValue("source", video.Link)
	videoWidget.AddAttributeValue("screenshot", video.Video.BigThumbnail)
	shareData := map[string]string{
		"uri":          video.Link,
		"content-type": "links",
	}
	videoWidget.AddAttributeValue("share-data", shareData)

	// Info
	info := scopes.NewPreviewWidget("info", "table")
	info.AddAttributeValue("title", "信息")
	table := [][]string{
		{"类型", video.Category},
		{"标签", video.Tags},
		{"发布时间", video.Published},
		{"总播放数", formatCount(video.ViewCount)},
		{"评论/收藏", fmt.Sprintf("%s / %s", video.CommentCount, video.FavoriteCount)},
		{"顶/踩", fmt.Sprintf("%s / %s", video.UpCount, video.DownCount)},
	}
	info.AddAttributeValue("values", table)

	// Expandable Content
	expandableWidget := scopes.NewPreviewWidget("expandable", "expandable")
	expandableWidget.AddAttributeValue("title", "截图")
	// Screenshots
	screenshots := scopes.NewPreviewWidget("screenshots", "gallery")
	links := []string{}
	for _, screenshot := range video.Screenshots {
		if screenshot.BigURL == "" {
			links = append(links, screenshot.URL)
		} else {
			links = append(links, screenshot.BigURL)
		}
	}
	screenshots.AddAttributeValue("sources", links)
	expandableWidget.AddWidget(screenshots)

	// Description
	description := scopes.NewPreviewWidget("description", "text")
	description.AddAttributeValue("title", "描述")
	var desText string
	if desText = video.Description; desText == "" {
		desText = "无"
	}
	description.AddAttributeValue("text", desText)

	// Actions
	actions := scopes.NewPreviewWidget("actions", "actions")
	acts := []map[string]string{
		{"id": "play", "label": "播放"},
	}
	actions.AddAttributeValue("actions", acts)

	// Expandable Comments
	expandableComments := scopes.NewPreviewWidget("comments", "expandable")
	expandableComments.AddAttributeValue("title", "评论")
	expandableComments.AddAttributeValue("collapsed-widgets", 2)

	logger.Println(sc, sc.ScopeSettings)

	// sometime setting is nil
	if sc.ScopeSettings == nil {
		logger.Println("setting: ", sc.ScopeSettings)
		// Get Settings
		var s settings
		err := sc.base.Settings(&s)
		if err != nil {
			logger.Println("[ERROR]", err)
			sc.ScopeSettings = &settings{50, 1, 20}
		} else {
			sc.ScopeSettings = &s
		}
	}

	// Comments
	videoComments := getCommentsByVideo(video.ID, int(sc.ScopeSettings.CommentCount))
	for _, comment := range videoComments {
		commentWidget := scopes.NewPreviewWidget("comment_"+comment.ID, "comment")
		switch {
		case strings.Contains(comment.Source.Name, "优酷"):
			commentWidget.AddAttributeValue("source", sc.base.ScopeDirectory()+"/icon.png")
		case strings.Contains(comment.Source.Name, "新浪"):
			commentWidget.AddAttributeValue("source", sc.base.ScopeDirectory()+"/data/weibo.png")
		case strings.Contains(comment.Source.Name, "ndroid"):
			commentWidget.AddAttributeValue("source", sc.base.ScopeDirectory()+"/data/android.png")
		case strings.Contains(comment.Source.Name, "iPhone"), strings.Contains(comment.Source.Name, "iPad"):
			commentWidget.AddAttributeValue("source", sc.base.ScopeDirectory()+"/data/apple.png")
		}
		commentWidget.AddAttributeValue("author", comment.User.Name)
		commentWidget.AddAttributeValue("subtitle", fmt.Sprintf("%s   %s", comment.Published, comment.Source.Name))
		commentWidget.AddAttributeValue("comment", comment.Content)
		expandableComments.AddWidget(commentWidget)
	}

	reply.PushWidgets(header, videoWidget, info, expandableWidget, description, actions, expandableComments)
}

func (sc *YoukuScope) viewShow(result *scopes.Result, reply *scopes.PreviewReply) {
	layoutOneCol := scopes.NewColumnLayout(1)
	layoutOneCol.AddColumn(
		"header",
		"show",
		"info",
		"expandable",
		"description",
		"actions",
	)
	layoutTwoCol := scopes.NewColumnLayout(2)
	layoutTwoCol.AddColumn(
		"header",
		"show",
		"actions",
	)
	layoutTwoCol.AddColumn(
		"info",
		"expandable",
		"description",
	)
	reply.RegisterLayout(layoutOneCol, layoutTwoCol)

	var showID string
	err := result.Get("show_id", &showID)
	if err != nil {
		logger.Println("[ERROR]", err)
		return
	}
	show := getShowDetail(showID)
	logger.Println("[SHOW PREVIEW]", showID, show.Name)

	// Header
	header := scopes.NewPreviewWidget("header", "header")
	header.AddAttributeValue("title", show.Name)
	header.AddAttributeValue("subtitle", fmt.Sprintf("评分: %.1f", formatScore(show.Score)))

	// Show
	showWidget := scopes.NewPreviewWidget("show", "video")
	showWidget.AddAttributeValue("source", show.PlayLink)
	if show.ThumbnailLarge != "" {
		showWidget.AddAttributeValue("screenshot", show.ThumbnailLarge)
	} else {
		showWidget.AddAttributeValue("screenshot", show.Thumbnail)
	}
	shareData := map[string]string{
		"uri":          show.PlayLink,
		"content-type": "links",
	}
	showWidget.AddAttributeValue("share-data", shareData)

	// Info
	info := scopes.NewPreviewWidget("info", "table")
	info.AddAttributeValue("title", "信息")
	table := [][]string{
		{"类型", show.Genre},
		{"地区", show.Area},
		{"上映", show.Released},
		{"更新至/总集数", fmt.Sprintf("%s / %s", fmt.Sprint(show.EpisodeUpdated), fmt.Sprint(show.EpisodeCount))},
		{"周播放/总播放", fmt.Sprintf("%s / %s", formatCount(show.ViewWeekCount), formatCount(show.ViewCount))},
		{"评论/收藏", fmt.Sprintf("%s / %s", formatCount(show.CommentCount), formatCount(show.FavoriteCount))},
		{"顶/踩", fmt.Sprintf("%s / %s", formatCount(show.UpCount), formatCount(show.DownCount))},
	}
	info.AddAttributeValue("values", table)

	// Description
	description := scopes.NewPreviewWidget("description", "text")
	description.AddAttributeValue("title", "描述")
	description.AddAttributeValue("text", show.Description)

	// Actions
	actions := scopes.NewPreviewWidget("actions", "actions")
	acts := []map[string]string{
		{"id": "play", "label": "分集播放"},
	}
	actions.AddAttributeValue("actions", acts)

	reply.PushWidgets(header, showWidget, info, description, actions)
}

func (sc *YoukuScope) queryVideo(keyword, departmentID string, reply *scopes.SearchReply) {

	logger.Printf("[QUERY VIDEOS] keyword: %s departmentID: %s\n", keyword, departmentID)

	var videoCategory string
	_deptIDs := strings.Split(departmentID, "_")
	if len(_deptIDs) > 1 {
		videoCategory = _deptIDs[1]
	}

	videos := queryVideosByKeyword(keyword, videoCategory, "history", "relevance", int(sc.ScopeSettings.ResultCount))

	category := reply.RegisterCategory("query_video", fmt.Sprintf("%s 相关%s视频", keyword, videoCategory), "", queryVideoTemplate)
	// Show Videos
	pushData(videos, category, reply)
}

func (sc *YoukuScope) queryShow(keyword, departmentID string, reply *scopes.SearchReply) {
	logger.Printf("[QUERY SHOWS] keyword: %s departmentID: %s\n", keyword, departmentID)

	var showCategory string
	_deptIDs := strings.Split(departmentID, "_")
	if len(_deptIDs) > 1 {
		showCategory = _deptIDs[1]
	}

	shows := queryShowsByKeyword(keyword, showCategory, 0, "view-couint", int(sc.ScopeSettings.ResultCount))

	category := reply.RegisterCategory("query_show", fmt.Sprintf("%s 相关%s节目", keyword, showCategory), "", queryVideoTemplate)

	// Show shows
	pushData(shows, category, reply)
}

func (sc *YoukuScope) showForAggregatedScopes(query *scopes.CannedQuery, metadata *scopes.SearchMetadata, reply *scopes.SearchReply) {
	logger.Println("[AGG]", metadata.AggregatedKeywords())

	rand.Seed(time.Now().UnixNano())

	queryString := query.QueryString()
	keywords := metadata.AggregatedKeywords()
	var showType, showCategory string

	if queryString != "" {
		sc.queryVideo(queryString, "", reply)
		// sc.queryShow(queryString, "", reply)
		return
	}

	switch {
	case isContainsKey("videos", keywords), isContainsKey("video", keywords):
		if rand.Intn(2) == 0 {
			showType = "video"
			categories := getVideoCategories(sc.base.ScopeDirectory())
			showCategory = categories[rand.Intn(len(categories))].Label
		} else {
			showType = "show"
			categories := getShowCategories(sc.base.ScopeDirectory())
			showCategory = categories[rand.Intn(len(categories))].Label
		}

	case isContainsKey("music", keywords):
		showType = "video"
		showCategory = "音乐"

	case isContainsKey("news", keywords):
		showType = "video"
		categories := []string{"资讯", "娱乐", "体育资讯", "游戏资讯"}
		showCategory = categories[rand.Intn(len(categories))]

	case isContainsKey("gaming", keywords):
		showType = "video"
		showCategory = "游戏"

	case isContainsKey("kids", keywords):
		showType = "video"
		categories := []string{"动漫", "亲子"}
		showCategory = categories[rand.Intn(len(categories))]

	case isContainsKey("educational", keywords):
		if rand.Intn(2) == 0 {
			showType = "video"
		} else {
			showType = "show"
		}
		showCategory = "教育"

	case isContainsKey("finance", keywords):
		showType = "video"
		showCategory = "财经资讯"

	case isContainsKey("humor", keywords):
		showType = "video"
		showCategory = "搞笑"

	case isContainsKey("lifestyle", keywords):
		showType = "video"
		showCategory = "生活"

	case isContainsKey("movies", keywords):
		if rand.Intn(2) == 0 {
			showType = "video"
		} else {
			showType = "show"
		}
		showCategory = "电影"
		categories := []string{"电影", "微电影"}
		if showType == "video" {
			showCategory = categories[rand.Intn(len(categories))]
		}

	case isContainsKey("science", keywords):
		showType = "video"
		showCategory = "科技"

	case isContainsKey("shopping", keywords):
		showType = "video"
		categories := []string{"时尚", "广告"}
		showCategory = categories[rand.Intn(len(categories))]

	case isContainsKey("sports", keywords):
		if rand.Intn(2) == 0 {
			showType = "video"
		} else {
			showType = "show"
		}
		showCategory = "体育"

	case isContainsKey("travel", keywords):
		showType = "video"
		showCategory = "旅游"

	case isContainsKey("tv", keywords):
		if rand.Intn(2) == 0 {
			showType = "video"
		} else {
			showType = "show"
		}
		categories := []string{"电视剧", "网剧", "综艺", "纪录片"}
		showCategory = categories[rand.Intn(len(categories))]

	case isContainsKey("comics", keywords):
		showType = "video"
		showCategory = "动漫"
	}

	category := reply.RegisterCategory("aggregate", fmt.Sprintf("%s", showCategory), "", fmt.Sprintf(custormVideoCategoryTemplate, itemSize))

	logger.Println("[Agg]", showType, showCategory)

	switch showType {
	case "video":
		videos := getVideosByCategory(showCategory, "", "today", "view-count", 1, 10)
		for _, video := range videos {
			result := scopes.NewCategorisedResult(category)
			result.SetTitle(video.Title)
			result.SetArt(video.Thumbnail)
			result.SetURI(video.Link)
			result.Set("attributes", []map[string]string{
				{"value": fmt.Sprintf("🕒%s", formatDuration(video.Duration))},
				{"value": fmt.Sprintf("🔥%s", formatCount(video.ViewCount))},
			})
			result.Set("video_id", video.ID)
			result.Set("type", "video")

			if err := reply.Push(result); err != nil {
				logger.Println("[ERROR]", err)
			}
		}
	case "show":
		shows := getShowsByCategory(showCategory, "", "view-today-count", 1, 10)
		for _, show := range shows {

			result := scopes.NewCategorisedResult(category)

			result.SetTitle(show.Name)
			result.SetArt(show.Thumbnail)
			result.SetURI(show.Link)
			result.Set("subtitle", fmt.Sprintf("更新 %s", fmt.Sprint(show.EpisodeUpdated)))
			result.Set("attributes", []map[string]string{
				{"value": fmt.Sprintf("★%.2f", formatScore(show.Score))},
				{"value": fmt.Sprintf("🔥%s", formatCount(show.ViewCount))},
			})
			result.Set("show_id", show.ID)
			result.Set("type", "show")

			if err := reply.Push(result); err != nil {
				logger.Println("[ERROR]", err)
			}
		}
	}

}

func pushData(data interface{}, category *scopes.Category, reply *scopes.SearchReply) {

	switch data.(type) {
	case []Video:
		videos := data.([]Video)
		for _, video := range videos {

			result := scopes.NewCategorisedResult(category)

			result.SetTitle(video.Title)
			result.SetArt(video.Thumbnail)
			result.SetURI(video.Link)
			result.Set("attributes", []map[string]string{
				{"value": fmt.Sprintf("🕒%s", formatDuration(video.Duration))},
				{"value": fmt.Sprintf("🔥%s", formatCount(video.ViewCount))},
			})
			result.Set("video_id", video.ID)
			result.Set("type", "video")

			if err := reply.Push(result); err != nil {
				logger.Println("[ERROR]", err)
			}
		}
	case []VideoDetail:
		videos := data.([]VideoDetail)
		for _, video := range videos {

			result := scopes.NewCategorisedResult(category)

			result.SetTitle(video.Title)
			result.SetArt(video.Thumbnail)
			result.SetURI(video.Link)
			result.Set("attributes", []map[string]string{
				{"value": fmt.Sprintf("🕒%s", formatDuration(video.Duration))},
				{"value": fmt.Sprintf("🔥%s", formatCount(video.ViewCount))},
			})
			result.Set("video_id", video.ID)
			result.Set("type", "video")

			if err := reply.Push(result); err != nil {
				logger.Println("[ERROR]", err)
			}
		}
	case []Show:
		shows := data.([]Show)
		for _, show := range shows {

			result := scopes.NewCategorisedResult(category)

			result.SetTitle(show.Name)
			result.SetArt(show.Thumbnail)
			result.SetURI(show.Link)
			result.Set("subtitle", fmt.Sprintf("更新 %s", fmt.Sprint(show.EpisodeUpdated)))
			result.Set("attributes", []map[string]string{
				{"value": fmt.Sprintf("★%.2f", formatScore(show.Score))},
				{"value": fmt.Sprintf("🔥%s", formatCount(show.ViewCount))},
			})
			result.Set("show_id", show.ID)
			result.Set("type", "show")

			if err := reply.Push(result); err != nil {
				logger.Println("[ERROR]", err)
			}
		}

	}

}

func formatCount(c interface{}) string {

	var text string
	count, _ := strconv.Atoi(fmt.Sprint(c))

	switch {
	case count <= 9999:
		text = fmt.Sprint(count)
	case count > 9999 && count <= 99999999:
		f := float64(count) / 10000
		text = fmt.Sprintf("%.2f万", f)
	case count > 99999999:
		f := float64(count) / 100000000
		text = fmt.Sprintf("%.2f亿", f)
	}

	return text
}

func formatDuration(data interface{}) string {
	durationString := fmt.Sprint(data)
	duration, err := strconv.ParseFloat(durationString, 64)
	if err != nil {
		logger.Println("[ERROR]", err)
	} else {
		hr := 0
		min := 0
		sec := 0
		sec = int(duration) % 60
		min = int(duration/60) % 60
		hr = int(duration/3600) % 60
		if hr == 0 {
			durationString = fmt.Sprintf("%d:%02d", min, sec)
		} else {
			durationString = fmt.Sprintf("%d:%02d:%02d", hr, min, sec)
		}
	}

	return durationString
}

func formatScore(s interface{}) float64 {
	f, _ := strconv.ParseFloat(fmt.Sprint(s), 64)
	return f
}

func isContainsKey(key string, keys []string) bool {
	result := false

	for _, v := range keys {
		if key == v {
			result = true
			break
		}
	}

	return result
}

func main() {

	logger.Println("Setting up accounts")
	watcher := accounts.NewWatcher(accountServiceType, []string{accountService})
	watcher.Settle()
	logger.Printf("Enabled services: %#v\n", watcher.EnabledServices())
	logger.Println("Starting scope")
	scope := &YoukuScope{
		Accounts: watcher,
	}

	if err := scopes.Run(scope); err != nil {
		log.Fatalln(err)
	}
}
