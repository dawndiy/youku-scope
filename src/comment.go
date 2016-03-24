package main

import (
	"encoding/json"
	"fmt"
	"net/http"
	"net/url"
)

// Comment struct to save video comment
type Comment struct {
	ID        string `json:"id"`
	Content   string `json:"content"`
	Published string `json:"published"`
	User      User   `json:"user"`
	Source    struct {
		Name string `json:"name"`
		Link string `json:"link"`
	} `json:"source"`
}

func getCommentsByVideo(videoID string, count int) []Comment {
	api := baseAPI + "comments/by_video.json"
	v := &url.Values{}
	v.Set("client_id", clientID)
	v.Set("video_id", videoID)
	v.Set("count", fmt.Sprint(count))
	api += "?" + v.Encode()

	res, err := http.Get(api)
	if err != nil {
		logger.Println("[ERROR]", err)
		return []Comment{}
	}
	defer res.Body.Close()

	var data struct {
		Total    int
		Comments []Comment `json:"comments"`
	}

	decoder := json.NewDecoder(res.Body)
	decoder.Decode(&data)

	return data.Comments
}
