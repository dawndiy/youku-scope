package main

import (
	"encoding/json"
	"fmt"
	"io/ioutil"
	"net/http"
	"net/url"
)

// Show information in Youku
type Show struct {
	ID             string      `json:"id"`
	Name           string      `json:"name"`
	Link           string      `json:"link"`
	PlayLink       string      `json:"play_link"`
	LastPlayLink   string      `json:"last_play_link"`
	Poster         string      `json:"poster"`
	Thumbnail      string      `json:"thumbnail"`
	SteamTypes     []string    `json:"streamtypes"`
	EpisodeCount   interface{} `json:"episode_count"`
	EpisodeUpdated interface{} `json:"episode_updated"`
	ViewCount      interface{} `json:"view_count"`
	Score          interface{} `json:"score"`
	Paid           int         `json:"paid"`
	Released       string      `json:"released"`
	Published      string      `json:"published"`
}

// ShowDetail to save detail information of show
type ShowDetail struct {
	Show
	PosterLarge        string      `json:"poster_large"`
	ThumbnailLarge     string      `json:"thumbnail_large"`
	Genre              string      `json:"genre"`
	Area               string      `json:"area"` // allow empty
	Category           string      `json:"category"`
	Description        string      `json:"description"` // allow empty
	Rank               interface{} `json:"rank"`
	ViewYesterdayCount interface{} `json:"view_yesterday_count"`
	ViewWeekCount      interface{} `json:"view_week_count"`
	CommentCount       interface{} `json:"comment_count"`
	FavoriteCount      interface{} `json:"favorite_count"`
	UpCount            interface{} `json:"up_count"`
	DownCount          interface{} `json:"down_count"`
}

// ShowCategory to save categories of shows
type ShowCategory struct {
	Term  string      `json:"term"`
	Label string      `json:"label"`
	Lang  string      `json:"lang"`
	Genre []ShowGenre `json:"genre"`
}

// ShowGenre to save genres of shows
type ShowGenre struct {
	Term  string `json:"term"`
	Label string `json:"label"`
	Lang  string `json:"lang"`
}

func getShowCategories(path string) []ShowCategory {

	f, err := ioutil.ReadFile(path + "/data/category.json")
	if err != nil {
		logger.Println("[ERROR]", err)
		return []ShowCategory{}
	}

	var data struct {
		Show []ShowCategory
	}

	err = json.Unmarshal(f, &data)
	if err != nil {
		logger.Println("[ERROR]", err)
		return []ShowCategory{}
	}
	return data.Show
}

func getShowsByCategory(category, genre, orderby string, page, count int) []Show {

	api := baseAPI + "shows/by_category.json"
	v := &url.Values{}
	v.Set("client_id", clientID)
	v.Set("category", category)
	v.Set("genre", genre)
	v.Set("orderby", orderby)
	v.Set("page", fmt.Sprint(page))
	v.Set("count", fmt.Sprint(count))
	api += "?" + v.Encode()

	// logger.Println(api)

	res, err := http.Get(api)
	if err != nil {
		logger.Println("[ERROR]", err)
		return []Show{}
	}
	defer res.Body.Close()

	var data struct {
		Total int
		Page  int
		Count int
		Shows []Show `json:"shows"`
	}

	decoder := json.NewDecoder(res.Body)
	decoder.Decode(&data)

	return data.Shows
}

func getShowDetail(showID string) ShowDetail {
	api := baseAPI + "shows/show.json"
	v := &url.Values{}
	v.Set("client_id", clientID)
	v.Set("show_id", showID)

	api += "?" + v.Encode()

	// logger.Println(api)

	res, err := http.Get(api)
	if err != nil {
		logger.Println("[ERROR]", err)
		return ShowDetail{}
	}
	defer res.Body.Close()

	var show ShowDetail

	decoder := json.NewDecoder(res.Body)
	err = decoder.Decode(&show)
	if err != nil {
		logger.Println("[ERROR]", "json parse", err)
	}

	return show
}

func queryShowsByKeyword(keyword, category string, unite int, orderby string, count int) []Show {
	api := baseAPI + "searches/show/by_keyword.json"
	v := &url.Values{}
	v.Set("client_id", clientID)
	v.Set("keyword", keyword)
	v.Set("category", category)
	v.Set("unite", fmt.Sprint(unite))
	v.Set("orderby", orderby)
	v.Set("count", fmt.Sprint(count))

	api += "?" + v.Encode()

	logger.Printf("[QUERY SHOWS] %s %s %s %d\n", keyword, category, orderby, count)

	res, err := http.Get(api)
	if err != nil {
		logger.Println("[ERROR]", err)
		return []Show{}
	}
	defer res.Body.Close()

	var data struct {
		Total int
		Shows []Show `json:"shows"`
	}

	decoder := json.NewDecoder(res.Body)
	decoder.Decode(&data)

	return data.Shows
}
