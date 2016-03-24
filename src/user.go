package main

// User is Youku User
type User struct {
	ID             int    `json:"id"`
	Name           string `json:"name"`
	Link           string `json:"link"`
	Avatar         string `json:"avatar"`
	AvatarLarge    string `json:"avatar_large"`
	Gender         string `json:"gender"`
	description    string `json:"description"`
	VideosCount    int    `json:"videos_count"`
	PlayListsCount int    `json:"playlists_count"`
	FavoritesCount int    `json:"favorites_count"`
	FollowersCount int    `json:"followers_count"`
	FollowingCount int    `json:"following_count"`
	StatusesCount  int    `json:"statuses_count"`
	SubscribeCount int    `json:"subscribe_count"`
	VVCount        int    `json:"vv_count"`
	RegistTime     string `json:"regist_time"`
}
