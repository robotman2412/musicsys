
# Type: Song metadata
```js
{
	// Song ID assigned by server.
	id: Number,
	// Displayed name.
	name: String,
	// HH:MM:SS representation of duration.
	str_duration: String,
	// Download progress where 1 is 100%.
	download_progress: Double,
	// Whether the server is busy converting the file for playback.
	is_converting: Boolean,
	// URL used for the icon shown in the song list.
	// May be external.
	icon_url: URL
}
```

# Type: Song queue metadata
```js
{
	// Song ID assigned by server.
	id: Number,
	// Index in the song queue where 0 is the first to ever get queued.
	index: Number
	// Displayed name; HTML escaped by server.
	name: String,
	// HH:MM:SS representation of duration.
	str_duration: String,
	// Download progress where 1 is 100%.
	download_progress: Double,
	// Whether the server is busy converting the file for playback.
	is_converting: Boolean,
	// URL used for the icon shown in the song list.
	// May be external.
	icon_url: URL
}
```

# Type: Playback metadata
```js
{
	// Song ID assigned by server.
	id: Number,
	// Displayed name; HTML escaped by server.
	name: String,
	// HH:MM:SS representation of duration.
	str_duration: String,
	// URL used for the icon shown in the song list.
	// May be external.
	icon_url: URL,
	// Current time in seconds of playback.
	current_time: Double,
	// Duration in seconds.
	duration: Double
}
```

# Type: Song submission
```js
{
	// URL for the server to download.
	url: String,
	// Name for the server to assign to this song.
	name: String
}
```

# Type: File upload initialiser
```js
{
	// Displayed name.
	name: String,
	// Reference number for client wanting to upload.
	// Only used once.
	ref: Number,
	// Size in bytes of the file to transfer.
	size: Integer
}
```

# Type: File upload status
```js
{
	// Type of status update; "start", "continue" or "done".
	status: String,
	// Reference number for client wanting to upload.
	// Only used once; for status == "start".
	ref: Number,
	// Song ID assigned by server.
	id: Number
}
```

# Type: File upload data
```js
{
	// Song ID assigned by server.
	id: Number,
	// Array of bytes containing file data.
	data: Array of Integer
}
```

# Message: song_meta
| Direction | Type
| --------- | ----
| S2C       | Song metadata

Indicates an update to a song in the song list.

# Message: song_in_queue_meta
| Direction | Type
| --------- | ----
| S2C       | Song metadata

Indicates a song added to the queue.

# Message: song_now_playing
| Direction | Type
| --------- | ----
| S2C       | Playback metadata

Indicates an update to the currently playing song.

# Message: now_playing_nothing
| Direction | Type
| --------- | ----
| S2C       | Any

Indicates that no song is currently playing.
This is not the same thing as being paused; there is no song to resume.

# Message: delete_song
| Direction | Type
| --------- | ----
| Both      | ID

Deletes the song with a given ID.

# Message: shuffle_mode
| Direction | Type
| --------- | ----
| Both      | Boolean

When true, the server will add a single, random song each time the queue depletes.

# Message: add_to_queue
| Direction | Type
| --------- | ----
| C2S       | ID

Adds the song with a given ID to the queue.

# Message: remove_from_queue
| Direction | Type
| --------- | ----
| Both      | ID

Removes from the song queue with given index.

# Message: set_volume
| Direction | Type
| --------- | ----
| Both      | Double

Sets the playback volume where 1 is 100%

# Message: playing
| Direction | Type
| --------- | ----
| S2C       | Boolean

Plays or pauses the current song

# Message: set_playing
| Direction | Type
| --------- | ----
| C2S       | Boolean

Plays or pauses the current song

# Message: song_question
| Direction | Type
| --------- | ----
| C2S       | String

Currently broken; Queries the name from a song link

# Message: song_answer
| Direction | Type
| --------- | ----
| S2C       | String

Currently broken: Reports the name from a song link

# Message: set_play_current_time
| Direction | Type
| --------- | ----
| C2S       | Double

Sets the playback time in seconds

# Message: set_volume
| Direction | Type
| --------- | ----
| Both      | Double

Sets the playback volume

# Message: skip
| Direction | Type
| --------- | ----
| C2S       | Any

When present, skip current song.

# Message: play_song_now
| Direction | Type
| --------- | ----
| C2S       | ID

Play the song with the given ID.

# Message: submit_song
| Direction | Type
| --------- | ----
| C2S       | Song submission

Submits a song link and name for the server to download.

# Message: upload_file_init
| Direction | Type
| --------- | ----
| C2S       | File upload initialiser

Initiates the process of uploading a media file to the server directly.

# Message: upload_prog
| Direction | Type
| --------- | ----
| S2C       | File upload status

Server response to the clien't request to upload a file.
This handles all status updates coming from the server.

When status == "start" or status == "continue", this is a promt for the clinet to send a chuck of data to the server.

# Message: upload_file_data
| Direction | Type
| --------- | ----
| C2S       | File upload data

Data sent by the client to transfer a file to the server.
This is a response to upload_prog messages where upload_prog.status == "start" or "continue".

# Message: fft_data
| Direction | Type
| --------- | ----
| S2C       | Array of Double

A list of doubles used to display a frequency analisys graph.
Typically about 100 long.
Smoothing is not applied; this is up to the client.

# Message: fft_color
| Direction | Type
| --------- | ----
| S2C       | 24-bit 0xRRGGBB color

Current foreground color for the FFT graph.
Smoothing is not applied; this is up to the client.

# Message: graph_style
| Direction | Type
| --------- | ----
| S2C       | Integer

The server cycles between 0, 1 and 2 on this number every time a new song is played.
It is used to select the style of graph:
1. 3D bar graph
2. 2D bar graph with peak indicators
3. Curved line graph
