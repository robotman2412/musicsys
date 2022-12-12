
# Type: Song metadata
```
{
	id: Number,
	name: String,
	str_duration: String,
	download_progress: Double,
	is_converting: Boolean,
	icon_url: URL
}
```

# Type: Song queue metadata
```
{
	id: Number,
	index: Number,
	name: String,
	str_duration: String,
	download_progress: Double,
	is_converting: Boolean,
	icon_url: URL
}
```

# Type: Playback metadata
```
{
	id: Number,
	name: String,
	str_duration: String,
	icon_url: URL,
	current_time: Double,
	duration: Double
}
```

# Type: Song submission
```
{
	url: String,
	name: String
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

Indicates an update to the currently playing song

# Message: delete_song
| Direction | Type
| --------- | ----
| Both      | ID

Deletes the song with a given ID.

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

Sets the playback volume

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

Queries the name from a song link

# Message: song_asnwer
| Direction | Type
| --------- | ----
| S2C       | String

Reports the name from a song link

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

When present, skip current song

# Message: play_song_now
| Direction | Type
| --------- | ----
| C2S       | ID

Play the song with the given ID

# Message: submit_song
| Direction | Type
| --------- | ----
| C2S       | Song submission

Submits a song for downloading.
