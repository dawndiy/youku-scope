package main

import (
	"encoding/json"
	"fmt"
	"io/ioutil"
	"net/http"
	"net/url"
)

// Video information from Youku
type Video struct {
	ID            string  `json:"id"`
	Title         string  `json:"title"`
	Link          string  `json:"link"`
	Thumbnail     string  `json:"thumbnail"`
	BigThumbnail  string  `json:"bigThumbnail"`
	Duration      float64 `json:"duration"`
	Category      string  `json:"category"`
	State         string  `json:"state"`
	ViewCount     int     `json:"view_count"`
	FavoriteCount int     `json:"favorite_count"`
	CommentCount  int     `json:"comment_count"`
	UpCount       int     `json:"up_count"`
	DownCount     int     `json:"down_count"`
	Published     string  `json:"published"`
	FavoriteTime  string  `json:"favorite_time"`
}

// VideoDetail to save detail information of video
type VideoDetail struct {
	Video
	BigThumbnail  string `json:"bitThumbnail"`
	Created       string `json:"created"`
	Duration      string `json:"duration"`
	Description   string `json:"description"`
	Player        string `json:"player"`
	PublicType    string `json:"public_type"`
	CopyrightType string `json:"copyright_type"`
	Tags          string `json:"tags"`
	Screenshots   []struct {
		Sequence int    `json:"seq"`
		URL      string `json:"url"`
		BigURL   string `json:"big_url"`
		SmallURL string `json:"small_url"`
		IsCover  int    `json:"is_cover"`
	} `json:"thumbnails"`
	FavoriteCount string `json:"favorite_count"`
	CommentCount  string `json:"comment_count"`
	UpCount       string `json:"up_count"`
	DownCount     string `json:"down_count"`
}

// VideoCategory to save categories of videos
type VideoCategory struct {
	ID     int
	Term   string
	Label  string
	Lang   string
	Genres []VideoGenres
}

// VideoGenres to save genres of videos
type VideoGenres struct {
	Term  string
	Label string
	Lang  string
}

func getVideoCategories(path string) []VideoCategory {

	f, err := ioutil.ReadFile(path + "/data/category.json")
	if err != nil {
		logger.Println("[ERROR]", err)
		return []VideoCategory{}
	}

	var data struct {
		Video []VideoCategory
	}

	err = json.Unmarshal(f, &data)
	if err != nil {
		logger.Println("[ERROR]", err)
		return []VideoCategory{}
	}
	return data.Video
}

func getVideosByCategory(category, genre, period, orderby string, page, count int) []Video {

	api := baseAPI + "videos/by_category.json"
	v := &url.Values{}
	v.Set("client_id", clientID)
	v.Set("category", category)
	v.Set("genre", genre)
	v.Set("period", period)
	v.Set("orderby", orderby)
	v.Set("page", fmt.Sprint(page))
	v.Set("count", fmt.Sprint(count))
	api += "?" + v.Encode()

	res, err := http.Get(api)
	if err != nil {
		logger.Println("[ERROR]", err)
		return []Video{}
	}
	defer res.Body.Close()

	var data struct {
		Total  int
		Page   int
		Count  int
		Videos []Video `json:"videos"`
	}

	decoder := json.NewDecoder(res.Body)
	decoder.Decode(&data)

	return data.Videos
}

func getVideoDetail(videoID string) VideoDetail {
	api := baseAPI + "videos/show.json"
	v := &url.Values{}
	v.Set("client_id", clientID)
	v.Set("video_id", videoID)
	v.Set("ext", "thumbnails")

	api += "?" + v.Encode()

	res, err := http.Get(api)
	if err != nil {
		logger.Println("[ERROR]", err)
		return VideoDetail{}
	}
	defer res.Body.Close()

	var video VideoDetail

	decoder := json.NewDecoder(res.Body)
	decoder.Decode(&video)

	return video
}

func queryVideosByKeyword(keyword, category, period, orderby string, count int) []VideoDetail {
	api := baseAPI + "searches/video/by_keyword.json"
	v := &url.Values{}
	v.Set("client_id", clientID)
	v.Set("keyword", keyword)
	v.Set("category", category)
	v.Set("period", period)
	v.Set("orderby", orderby)
	v.Set("count", fmt.Sprint(count))

	api += "?" + v.Encode()

	logger.Printf("[QUERY VIDEOS] %s %s %s %s %d\n", keyword, category, period, orderby, count)

	res, err := http.Get(api)
	if err != nil {
		logger.Println("[ERROR]", err)
		return []VideoDetail{}
	}
	defer res.Body.Close()

	var data struct {
		Total  int
		Videos []VideoDetail `json:"videos"`
	}

	decoder := json.NewDecoder(res.Body)
	decoder.Decode(&data)

	return data.Videos

}
