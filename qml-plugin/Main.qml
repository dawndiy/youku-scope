import Ubuntu.OnlineAccounts.Plugin 1.0

OAuthMain {
    creationComponent: OAuth {
        function completeCreation(reply) {
            var http = new XMLHttpRequest();
            var url = "https://openapi.youku.com/v2/users/myinfo.json?client_id=YOUR_CLIENT_ID&access_token=" + reply.AccessToken;
            console.log('Access Token: '+ reply.AccessToken)
            http.open("GET", url, true);
            http.onreadystatechange = function() {
                if (http.readyState === 4) {
                    if (http.status === 200) {
                        console.log("ok");
                        console.log("response text: " + http.responseText);
                        var response = JSON.parse(http.responseText);
                        account.updateDisplayName(response.name);
                        account.synced.connect(finished);
                        account.sync();
                    } else {
                        console.log("error: " + http.status)
                        cancel();
                    }
                }
            };
            http.send(null);
        }
    }
}

