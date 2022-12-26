
var ws;
var songs = {};
var song_queue = {};
var songs_queue_playing = {};
var magnitudes = [];
var sum = [];
var psum = [];
for (var i = 0; i < 100; i++) {
	sum[i] = magnitudes[i] = psum[i] = 0;
}
var fft_color= 0x0000ff;
var mouseX = 0;
var mouseY = 0;
var mouseButton = -1;
var playing_present = false;
var now_playing_meta;
var main_playing;
var fft_interval;
var graphStyle = 0;
var is_mobile_user;
var last_click_time;
var last_song_question;
var current_title;
var song_queue_ht = 0;
var songs_ht = 0;
var resize_inc = 0.075;

function isMobileUser() {
  let check = false;
  (function(a){if(/(android|bb\d+|meego).+mobile|avantgo|bada\/|blackberry|blazer|compal|elaine|fennec|hiptop|iemobile|ip(hone|od)|iris|kindle|lge |maemo|midp|mmp|mobile.+firefox|netfront|opera m(ob|in)i|palm( os)?|phone|p(ixi|re)\/|plucker|pocket|psp|series(4|6)0|symbian|treo|up\.(browser|link)|vodafone|wap|windows ce|xda|xiino/i.test(a)||/1207|6310|6590|3gso|4thp|50[1-6]i|770s|802s|a wa|abac|ac(er|oo|s\-)|ai(ko|rn)|al(av|ca|co)|amoi|an(ex|ny|yw)|aptu|ar(ch|go)|as(te|us)|attw|au(di|\-m|r |s )|avan|be(ck|ll|nq)|bi(lb|rd)|bl(ac|az)|br(e|v)w|bumb|bw\-(n|u)|c55\/|capi|ccwa|cdm\-|cell|chtm|cldc|cmd\-|co(mp|nd)|craw|da(it|ll|ng)|dbte|dc\-s|devi|dica|dmob|do(c|p)o|ds(12|\-d)|el(49|ai)|em(l2|ul)|er(ic|k0)|esl8|ez([4-7]0|os|wa|ze)|fetc|fly(\-|_)|g1 u|g560|gene|gf\-5|g\-mo|go(\.w|od)|gr(ad|un)|haie|hcit|hd\-(m|p|t)|hei\-|hi(pt|ta)|hp( i|ip)|hs\-c|ht(c(\-| |_|a|g|p|s|t)|tp)|hu(aw|tc)|i\-(20|go|ma)|i230|iac( |\-|\/)|ibro|idea|ig01|ikom|im1k|inno|ipaq|iris|ja(t|v)a|jbro|jemu|jigs|kddi|keji|kgt( |\/)|klon|kpt |kwc\-|kyo(c|k)|le(no|xi)|lg( g|\/(k|l|u)|50|54|\-[a-w])|libw|lynx|m1\-w|m3ga|m50\/|ma(te|ui|xo)|mc(01|21|ca)|m\-cr|me(rc|ri)|mi(o8|oa|ts)|mmef|mo(01|02|bi|de|do|t(\-| |o|v)|zz)|mt(50|p1|v )|mwbp|mywa|n10[0-2]|n20[2-3]|n30(0|2)|n50(0|2|5)|n7(0(0|1)|10)|ne((c|m)\-|on|tf|wf|wg|wt)|nok(6|i)|nzph|o2im|op(ti|wv)|oran|owg1|p800|pan(a|d|t)|pdxg|pg(13|\-([1-8]|c))|phil|pire|pl(ay|uc)|pn\-2|po(ck|rt|se)|prox|psio|pt\-g|qa\-a|qc(07|12|21|32|60|\-[2-7]|i\-)|qtek|r380|r600|raks|rim9|ro(ve|zo)|s55\/|sa(ge|ma|mm|ms|ny|va)|sc(01|h\-|oo|p\-)|sdk\/|se(c(\-|0|1)|47|mc|nd|ri)|sgh\-|shar|sie(\-|m)|sk\-0|sl(45|id)|sm(al|ar|b3|it|t5)|so(ft|ny)|sp(01|h\-|v\-|v )|sy(01|mb)|t2(18|50)|t6(00|10|18)|ta(gt|lk)|tcl\-|tdg\-|tel(i|m)|tim\-|t\-mo|to(pl|sh)|ts(70|m\-|m3|m5)|tx\-9|up(\.b|g1|si)|utst|v400|v750|veri|vi(rg|te)|vk(40|5[0-3]|\-v)|vm40|voda|vulc|vx(52|53|60|61|70|80|81|83|85|98)|w3c(\-| )|webc|whit|wi(g |nc|nw)|wmlb|wonu|x700|yas\-|your|zeto|zte\-/i.test(a.substr(0,4))) check = true;})(navigator.userAgent||navigator.vendor||window.opera);
  return check;
};

function tableSize(table) {
    i = 0;
    for (x in table) i++;
    return i;
}

window.onbeforeunload = onUnload;
function onLoad() {
	if (is_mobile_user = isMobileUser()) {
		document.body.setAttribute("class", "mobile");
		var centerElem = document.getElementById("center");
		centerElem.style.left = "0";
		centerElem.style.right = "0";
	}
	start_web_soc();
	add_mouse_listener();
	add_keyboard_listener();
	setInterval(fft_loop, 1000 / 60);
	setInterval(resizing_loop, 1000 / 60);
}

function onUnload() {
	ws.close();
}

function resizing_loop() {
	var songQueueHolder = document.getElementById("song_queue_holder");
	var songsHolder = document.getElementById("song_holder");
	var mul = songQueueHolder.getClientRects()[0].width / 16 * 9;
	var div;
	if (is_mobile_user) div = 2;
	else div = 3;
	song_queue_ht += (Math.ceil(tableSize(song_queue) / div) - song_queue_ht) * resize_inc;
	songs_ht += (Math.ceil(tableSize(songs) / div) - songs_ht) * resize_inc;
	songQueueHolder.style.height = song_queue_ht * mul / div + "px";
	songsHolder.style.height = songs_ht * mul / div + "px";
}

function fft_loop() {
	var canvas = document.getElementById("fft_canvas");
	var width = Math.floor(canvas.getClientRects()[0].width);
	if (canvas.width != width) {
		canvas.width = width;
		canvas.height = width / 16 * 9;
	}
	var ctx = canvas.getContext("2d");
	var numBars = magnitudes.length;
	var barWidth = canvas.width / numBars;
	
	var B = fft_color & 0xff;
	var G = (fft_color >> 8) & 0xff;
	var R = (fft_color >> 16) & 0xff;
	var scale = 0.02;
	
	ctx.fillStyle = "rgba(" + R/8 + "," + G/8 + "," + B/8 + ",0.19)";
	ctx.fillRect(0, 0, canvas.width, canvas.height);
	var imgd = ctx.getImageData(canvas.width - 1, 0, 1, 1);
	var pix = imgd.data;
	document.body.style = "background-color:rgb(" + pix[0] + "," + pix[1] + "," + pix[2] + ")";
	
	graphStyle %= 3;
	ctx.fillStyle = "rgba(" + R + "," + G + "," + B + ",1)";
	
	if (!main_playing) {
		for (i = 0; i < numBars; i++) {
			sum[i] *= 0.85;
		}
	}
	
	if (graphStyle == 0) {
		ctx.fillStyle = "rgba(" + R*0.75 + "," + G*0.75 + "," + B*0.75 + ",1)";
		for (i = 0; i < numBars; i++) {
			ctx.fillRect(i * barWidth + barWidth * 0.3, canvas.height, barWidth, -sum[i] * scale * canvas.height - barWidth * 0.3);
		}
		ctx.fillStyle = "rgba(" + R + "," + G + "," + B + ",1)";
		for (i = 0; i < numBars; i++) {
			ctx.fillRect(i * barWidth, canvas.height, barWidth, -sum[i] * scale * canvas.height);
		}
	} else if (graphStyle == 1) {
		for (i = 0; i < numBars; i++) {
			if (!isFinite(psum[i])) psum[i] = 0;
			psum[i] = Math.max(sum[i], psum[i] - 0.125);
			ctx.fillRect(i * barWidth, canvas.height, barWidth, -sum[i] * scale * canvas.height);
			ctx.fillRect(i * barWidth, canvas.height - psum[i] * scale * canvas.height, barWidth, barWidth * 0.5);
		}
		
		ctx.fillStyle = "rgba(0,0,0,0.125)";
		//ctx.fillStyle = "rgb(0,0,0)";
		//ctx.lineWidth = 0.5;
		var iY = Math.ceil(barWidth * 0.5);
		for (y = -1; y < canvas.height; y += iY) {
			//ctx.moveTo(0, y);
			//ctx.lineTo(canvas.width, y);
			ctx.fillRect(0, y, canvas.width, 1);
		}
	} else if (graphStyle == 2) {
		ctx.strokeStyle = "rgba(" + R + "," + G + "," + B + ",1)";
		ctx.lineWidth = barWidth * 0.5;
		// move to the first point
		ctx.beginPath()
		ctx.moveTo(barWidth * -0.5, canvas.height);
		var i;
		for (i = 0; i < numBars - 1; i ++) {
			var x0 = i * barWidth + 0.5 * barWidth;
			var x1 = x0 + barWidth;
			var y0 = canvas.height - sum[i] * scale * canvas.height;
			var y1 = canvas.height - sum[i + 1] * scale * canvas.height;
			var xc = (x0 + x1) / 2;
			var yc = (y0 + y1) / 2;
			ctx.quadraticCurveTo(x0, y0, xc, yc);
		}
		// curve through the last two points
		var x = i * barWidth + 0.5 * barWidth;
		var y = canvas.height - sum[i] * scale * canvas.height;
		ctx.quadraticCurveTo(x, y, canvas.width + barWidth * 0.5, canvas.height);
		ctx.stroke();
		lineWidth = 1;
	}
}

function escapeHtml(unsafe) {
    return unsafe
         .replace(/&/g, "&amp;")
         .replace(/</g, "&lt;")
         .replace(/>/g, "&gt;")
         .replace(/"/g, "&quot;")
         .replace(/'/g, "&#039;");
 }

function shuffle(array) {
  var currentIndex = array.length, temporaryValue, randomIndex;

  // While there remain elements to shuffle...
  while (0 !== currentIndex) {

    // Pick a remaining element...
    randomIndex = Math.floor(Math.random() * currentIndex);
    currentIndex -= 1;

    // And swap it with the current element.
    temporaryValue = array[currentIndex];
    array[currentIndex] = array[randomIndex];
    array[randomIndex] = temporaryValue;
  }

  return array;
}

function update_shuffle_mode() {
	ws.send(JSON.stringify({shuffle_mode: document.getElementById("shuffle_mode").checked}));
}

function restart_web_soc() {
	//dirty approach
	//document.location.reload();
	
	// Close WS if possible.
	try {
		ws.onclose = ()=>{};
		ws.onerror = ()=>{};
		ws.close();
	} catch(x) {
		// console.error(x);
	}
	
	// Delete all dynamically loaded content.
	document.getElementById("now_playing_holder").innerHTML = "";
	document.getElementById("song_queue_holder").innerHTML = "";
	document.getElementById("song_holder").innerHTML = "";
	set_title("Music System");
	
	// Reset state.
	songs = {};
	song_queue = {};
	playing_present = false;
	main_playing = false;
	
	setTimeout(start_web_soc, 1000);
}

function start_web_soc() {
	if ("WebSocket" in window) {
		ws = new WebSocket("ws://" + document.location.hostname + ":6969/");
		
		ws.onopen = function() {
			
		};

		ws.onmessage = function (evt) {
			// console.log(evt.data);
			var data = JSON.parse(evt.data);
			handle_message(data);
		};

		ws.onclose = restart_web_soc;
		
		ws.onerror = restart_web_soc;
	}
	else
	{
		alert("Your browser does not support web sockets, or has them disabled.\nThe Music System will not work.");
	}
}

function delete_song(id) {
	if (!songs[id].is_converting && confirm("You are about to delete the song:\n" + songs[id].name + "\nAre you sure you want to continue?")) {
		data = {};
		data.delete_song = id;
		ws.send(JSON.stringify(data));
	}
}

function handle_message(data) {
	if (data.song_meta != undefined) {
		songs[data.song_meta.id] = data.song_meta;
		handle_meta(data.song_meta);
	}
	if (data.song_in_queue_meta != undefined) {
		qhandle_meta(data.song_in_queue_meta);
	}
	if (data.song_now_playing != undefined) {
		phandle_meta(data.song_now_playing);
	}
	if (data.delete_song != undefined) {
		var meta = songs[data.delete_song];
		songs[data.delete_song].is_removed = true;
		toRemove = document.getElementById("song_" + meta.id);
		toRemove.setAttribute("class", "song-o-remove");
		var min_id = data.delete_song + 1;
		var offs = 0;
		for (id in songs) {
			var _meta = songs[id];
			if (id >= min_id) {
				var thing = document.getElementById("song_" + id);
				if (is_mobile_user) {
					thing.setAttribute("class", ((offs % 2) == 0) ? "song-o wrap-left" : "song-o move-left");
				} else {
					thing.setAttribute("class", ((offs % 3) == 0) ? "song-o wrap-left" : "song-o move-left");
				}
			}
			offs ++;
		}
		min_id --;
		setTimeout(function() {
			toRemove.parentNode.removeChild(toRemove);
			for (id in songs) {
				var _meta = songs[id];
				if (id >= min_id) {
					var thing = document.getElementById("song_" + id);
					thing.setAttribute("class", "song-o");
				}
			}
		}, 490);
		delete songs[data.delete_song];
	}
	if (data.remove_from_queue != undefined) {
		var meta = song_queue[data.remove_from_queue];
		song_queue[data.remove_from_queue].is_removed = true;
		var toRemove = document.getElementById("qsong_" + meta.index);
		toRemove.setAttribute("class", "song-o-remove");
		var min_index = data.remove_from_queue + 1;
		var offs = 0;
		for (index in song_queue) {
			var _meta = song_queue[index];
			if (index >= min_index) {
				var thing = document.getElementById("qsong_" + index);
				if (is_mobile_user) {
					thing.setAttribute("class", ((offs % 2) == 0) ? "song-o wrap-left" : "song-o move-left");
				} else {
					thing.setAttribute("class", ((offs % 3) == 0) ? "song-o wrap-left" : "song-o move-left");
				}
			}
			offs ++;
		}
		min_index --;
		setTimeout(function() {
			toRemove.parentNode.removeChild(toRemove);
			for (index in song_queue) {
				var _meta = song_queue[index];
				if (index >= min_index) {
					var thing = document.getElementById("qsong_" + index);
					thing.setAttribute("class", "song-o");
				}
			}
		}, 490);
		delete song_queue[data.remove_from_queue];
	}
	if (data.set_volume != undefined) {
		var volumeKnob = document.getElementById("volume_knob");
		var volumeTail = document.getElementById("volume_tail");
		volumeKnob.style.left = (data.set_volume * 100) + "%";
		volumeTail.style.width = (data.set_volume * 100) + "%";
	}
	if (data.playing != undefined) {
		main_playing = data.playing;
		play0 = document.getElementById("song_play0");
		play1 = document.getElementById("song_play1");
		if (play0 && play1) {
			if (!main_playing) {
				play0.setAttribute("class", "song-play-0");
				play1.setAttribute("class", "song-play-1");
			}
			else
			{
				play0.setAttribute("class", "song-pause-0");
				play1.setAttribute("class", "song-pause-1");
			}
		}
	}
	if (data.now_playing_nothing != undefined) {
		set_title("Music System");
		playing_present = false;
		var timeKnob = document.getElementById("time_knob");
		var timeTail = document.getElementById("time_tail");
		timeKnob.style.left = "100%";
		timeTail.style.width = "100%";
		var timeText = document.getElementById("play_timer");
		timeText.innerHTML = "00:00 / 00:00";
	}
	if (data.song_answer != undefined) {
		var song_name = document.getElementById("song_name");
		song_name.value = data.song_answer;
	}
	if (data.fft_data != undefined) {
		magnitudes = data.fft_data;
		for (i = 0; i < magnitudes.length; i++) {
			if (!isFinite(sum[i])) {
				sum[i] = 0;
			}
			sum[i] += (magnitudes[i] - sum[i]) * 0.07;
			// sum[i] = magnitudes[i]/3;
		}
	}
	if (data.fft_color != undefined) {
		fft_color = data.fft_color;
	}
	if (data.graph_style != undefined) {
		graphStyle = data.graph_style;
	}
	if (data.upload_prog != undefined) {
		handle_upload_message(data.upload_prog);
	}
	if (data.shuffle_mode != undefined) {
		document.getElementById("shuffle_mode").checked = data.shuffle_mode;
	}
}

function handle_meta(meta) {
	if (document.getElementById("song_" + meta.id) == null) {
		var split = document.getElementById("song_template").innerHTML.split("meta");
		var template = split[0];
		for (i = 1; i < split.length; i++) {
			m0 = split[i].indexOf("[");
			m1 = split[i].indexOf("]");
			if (m0 < 0 || m1 < 0) {
				template += "meta" + split[i];
			}
			var property = split[i].substring(m0 + 1, m1);
			template += meta[property] + split[i].substring(m1 + 1);
		}
		document.getElementById("templater").innerHTML += template;
		var songElem = document.getElementById("song_" + meta.id);
		document.getElementById("song_holder").appendChild(songElem);
		setTimeout(function () {
			document.getElementById("song_" + meta.id).setAttribute("class", "song-o");
		}, 990);
	}
	load = document.getElementById("song_" + meta.id + "_load");
	load_outer = document.getElementById("song_" + meta.id + "_load_outer");
	load_text = document.getElementById("song_" + meta.id + "_load_text");
	delete_it = document.getElementById("song_" + meta.id + "_delete");
	button_left = document.getElementById("song_" + meta.id + "_button_left");
	button_center = document.getElementById("song_" + meta.id + "_button_center");
	duration = document.getElementById("song_" + meta.id + "_duration");
	percent = Math.round(meta.download_progress * 100);
	if (meta.download_progress < 1) {
		load.setAttribute("style", "width: " + percent + "%;");
		load_text.innerHTML = "uploading - " + percent + "%";
	}
	else if (meta.is_converting) {
		load.setAttribute("class", "song-load-i-conv");
		load.setAttribute("style", "width: 100%;");
		load_text.innerHTML = "converting";
	}
	else
	{
		load.innerHTML = "done";
		load_outer.setAttribute("class", "song-load-o fade");
		//delete_it.setAttribute("class", "fa fa-trash-o song-delete fadein");
		button_left.setAttribute("class", "song-add fadein");
		button_center.setAttribute("class", "song-play fadein");
		duration.innerHTML = meta.str_duration;
		setTimeout(function () {
			//load_outer.setAttribute("class", "hidden");
			//delete_it.setAttribute("class", "fa fa-trash-o song-delete");
			button_left.setAttribute("class", "song-add");
			button_center.setAttribute("class", "song-play");
		}, 490);
		load.setAttribute("style", "width: 100%;");
	}
}

function phandle_meta(meta) {
	now_playing_meta = meta;
	playing_present = true;
	set_title(meta.name);
	var timePercent = meta.current_time / meta.duration * 100;
	var timeKnob = document.getElementById("time_knob");
	var timeTail = document.getElementById("time_tail");
	timeKnob.style.left = timePercent + "%";
	timeTail.style.width = timePercent + "%";
	var timeText = document.getElementById("play_timer");
	timeText.innerHTML = format_time(meta.current_time) + " / " + format_time(meta.duration);
}

function format_time(seconds) {
	var minutes = Math.floor(seconds / 60);
	seconds %= 60;
	var hours = Math.floor(minutes / 60);
	minutes %= 60;
	if (hours > 0) {
		return format_02d(hours) + ":" + format_02d(minutes) + ":" + format_02d(seconds);
	} else {
		return format_02d(minutes) + ":" + format_02d(seconds);
	}
}

function format_02d(num) {
	num = Math.floor(num);
	if (num < 10) {
		return "0" + num;
	} else {
		return "" + num;
	}
}

function big_play_pause() {
	if (!is_mobile_user) {
		play_pause();
	}
}

function play_pause() {
	main_playing = !main_playing;
	data = {};
	data.set_playing = main_playing;
	ws.send(JSON.stringify(data));
}

function update_time() {
	var timeCalc = document.getElementById("time_calc");
	var rects = timeCalc.getClientRects();
	var x = rects[0].x;
	var width = rects[0].width;
	var time;
	if (now_playing_meta == undefined) {
		return;
	}
	if (mouseX < x) {
		time = 0;
	} else if (mouseX > x + width) {
		time = 1;
	} else {
		time = (mouseX - x) / width;
	}
	time *= now_playing_meta.duration;
	data = {};
	data.set_play_current_time = time;
	ws.send(JSON.stringify(data));
}

function update_volume() {
	var volumeCalc = document.getElementById("volume_calc");
	var rects = volumeCalc.getClientRects();
	var x = rects[0].x;
	var width = rects[0].width;
	var value;
	if (now_playing_meta == undefined) {
		return;
	}
	if (mouseX < x) {
		value = 0;
	} else if (mouseX > x + width) {
		value = 1;
	} else {
		value = (mouseX - x) / width;
	}
	data = {};
	data.set_volume = value;
	ws.send(JSON.stringify(data));
}

function skip_song() {
	data = {};
	data.skip = true;
	ws.send(JSON.stringify(data));
}

function play_song(id) {
	data = {};
	data.play_song_now = id;
	ws.send(JSON.stringify(data));
}

function qhandle_meta(meta) {
	if (document.getElementById("qsong_" + meta.index) == null) {
		console.log("add to queue");
		var split = document.getElementById("song_in_queue_template").innerHTML.split("meta");
		var template = split[0];
		for (i = 1; i < split.length; i++) {
			m0 = split[i].indexOf("[");
			m1 = split[i].indexOf("]");
			if (m0 < 0 || m1 < 0) {
				template += "meta" + split[i];
			}
			var property = split[i].substring(m0 + 1, m1);
			template += meta[property] + split[i].substring(m1 + 1);
		}
		document.getElementById("templater").innerHTML += template;
		var songElem = document.getElementById("qsong_" + meta.index);
		document.getElementById("song_queue_holder").appendChild(songElem);
		setTimeout(function () {
			document.getElementById("qsong_" + meta.index).setAttribute("class", "song-o");
		}, 490);
		song_queue[meta.index] = meta;
		songs_queue_playing[meta.id] = false;
	}
	/*load = document.getElementById("qsong_" + meta.id + "_load");
	load_outer = document.getElementById("qsong_" + meta.id + "_load_outer");
	delete_it = document.getElementById("qsong_" + meta.id + "_delete");
	percent = Math.round(meta.download_progress * 100);
	if (meta.download_progress < 1) {
		load.setAttribute("style", "width: " + percent + "%;");
		load.innerHTML = "uploading - " + percent + "%";
	}
	else if (meta.is_converting) {
		load.setAttribute("class", "song-load-i-conv");
		load.innerHTML = "converting";
		load.setAttribute("style", "width: 100%;");
	}
	else
	{
		load.innerHTML = "done";
		load_outer.setAttribute("class", "song-load-o fade");
		delete_it.setAttribute("class", "fa fa-trash-o song-delete fadein");
		setTimeout(function () {
			load_outer.setAttribute("class", "hidden");
			delete_it.setAttribute("class", "fa fa-trash-o song-delete");
		}, 1000);
		load.setAttribute("style", "width: 100%;");
	}*/
}

function clr_err(what) {
	document.getElementById(what).setAttribute("class", "wide");
}

function song_question() {
	var song_url = document.getElementById("song_url");
	if (last_song_question != song_url.value) {
		last_song_question = song_url.value;
		data = {
			song_question: song_url.value
		};
		ws.send(JSON.stringify(data));
	}
}

function submit_song() {
	song_name = document.getElementById("song_name");
	song_url = document.getElementById("song_url");
	if (song_name.value === "") {
		song_name.setAttribute("class", "wide_err");
	}
	if (song_url.value === "") {
		song_url.setAttribute("class", "wide_err");
	}
	if (song_name.value === "" || song_url.value === "") {
		return;
	}
	song_url.setAttribute("class", "wide");
	song_name.setAttribute("class", "wide");
	submit = {};
	submit.name = song_name.value;
	submit.url = song_url.value;
	data = {};
	data.submit_song = submit;
	ws.send(JSON.stringify(data));
	song_name.value = "";
	song_url.value = "";
}

function add_mouse_listener() {
    document.onmousemove = handleMouseMove;
    function handleMouseMove(event) {
        var eventDoc, doc, body;

        event = event || window.event; // IE-ism
		
        // If pageX/Y aren't available and clientX/Y are,
        // calculate pageX/Y - logic taken from jQuery.
        // (This is to support old IE)
        if (event.pageX == null && event.clientX != null) {
            eventDoc = (event.target && event.target.ownerDocument) || document;
            doc = eventDoc.documentElement;
            body = eventDoc.body;

            event.pageX = event.clientX +
              (doc && doc.scrollLeft || body && body.scrollLeft || 0) -
              (doc && doc.clientLeft || body && body.clientLeft || 0);
            event.pageY = event.clientY +
              (doc && doc.scrollTop  || body && body.scrollTop  || 0) -
              (doc && doc.clientTop  || body && body.clientTop  || 0);
        }
		
        // Use event.pageX / event.pageY here
		mouseX = event.pageX;
		mouseY = event.pageY;
    }
}

function add_keyboard_listener() {
	document.onkeydown = (e)=>{
		if (e.key == " " && document.activeElement.tagName != "INPUT") {
			play_pause();
			e.preventDefault();
		}
	}
}

function add_to_queue(id) {
	data = {};
	data.add_to_queue = id;
	ws.send(JSON.stringify(data));
}

function remove_from_queue(index) {
	data = {};
	data.remove_from_queue = index;
	ws.send(JSON.stringify(data));
}

function qplay_song(index) {
	meta = song_queue[index];
	remove_from_queue(index);
	play_song(meta.id);
}

function clear_search() {
	for (id in songs) {
		var meta = songs[id];
		var songElem = document.getElementById("song_" + id);
		var _class = songElem.getAttribute("class");
		if (!meta.is_removed) {
			if (_class == "song-o-remove") songElem.setAttribute("class", "song-o-new");
		}
	}
	for (index in song_queue) {
		var meta = song_queue[index];
		var id = meta.id;
		if (!meta.is_removed) {
			var songElem = document.getElementById("qsong_" + index);
			var _class = songElem.getAttribute("class");
			if (_class == "song-o-remove") songElem.setAttribute("class", "song-o-new");
		}
	}
}

function search(query) {
	query = query.toLowerCase().trim();
	if (query.length == 0) {
		clear_search();
		return;
	}
	queries = query.split(" ");
	for (id in songs) {
		var meta = songs[id];
		if (!meta.is_removed) {
			var match = search_match(queries, meta.name);
			var songElem = document.getElementById("song_" + id);
			var _class = songElem.getAttribute("class");
			if (!match) {
				if (_class != "song-o-remove") songElem.setAttribute("class", "song-o-remove");
			} else {
				if (_class == "song-o-remove") songElem.setAttribute("class", "song-o-new");
			}
		}
	}
	for (index in song_queue) {
		var meta = song_queue[index];
		var id = meta.id;
		if (!meta.is_removed) {
			var match = search_match(queries, meta.name);
			var songElem = document.getElementById("qsong_" + index);
			var _class = songElem.getAttribute("class");
			if (!match) {
				if (_class != "song-o-remove") songElem.setAttribute("class", "song-o-remove");
			} else {
				if (_class == "song-o-remove") songElem.setAttribute("class", "song-o-new");
			}
		}
	}
}

function search_match(queries, name) {
	name = name.toLowerCase();
	var num = 0;
	for (i in queries) {
		if (queries[i].length > 0 && name.includes(queries[i])) {
			num ++;
		}
	}
	return num;
}

function set_title(title) {
	if (current_title != title) {
		var titleElem = document.getElementById("title_elem");
		var titleH1 = document.getElementById("title_h1");
		titleElem.innerHTML = titleH1.innerHTML = title;
		current_title = title;
	}
}

function add_songs_for_time() {
	var timeInput = document.getElementById("add_songs_for_time");
	var timeStr = timeInput.value.trim();
	if (timeStr.match(/^([0-9]{1,2}:){0,3}[0-9]{1,2}$/)) {
		var songs_to_add = songs_for_time(str_duration_to_seconds(timeStr));
		//console.log(songs_to_add);
		shuffle(songs_to_add);
		for (i in songs_to_add) {
			add_to_queue(songs_to_add[i].id);
		}
	}
}

var last_songs_timed = [];
var last_songs_duration;

function songs_for_time(time, max_deviance) {
	if (!max_deviance) max_deviance = Math.floor(time * 0.025);
	var min_time = time - max_deviance;
	var max_time = time + max_deviance;
	for (id in songs) {
		if (songs[id].duration == undefined) {
			songs[id].duration = str_duration_to_seconds(songs[id].str_duration);
		}
	}
	var find_list = [];
	var valid_lists = [{time:0,list:[]}];
	var excluded = Array.from(last_songs_timed);
	var find_mode = find_song_closest;
	var total_time = 0;
	var max_iterations = 1000;
	var iteration = 0;
	while (true) {
		var find_time = time - total_time;
		var found = find_song(find_time, find_mode, excluded);
		if (found && total_time + found.duration > max_time) found = null;
		console.log(Array.from(excluded), find_mode, found, Array.from(find_list));
		if (!found) {
			if (find_mode == find_song_closest) {
				find_mode = find_song_shortest;
				valid_lists.push({time: total_time, list: Array.from(find_list)});
				//excluded.pop();
				var rm = find_list.pop();
				if (rm) total_time -= rm.duration;
			} else if (find_mode == find_song_shortest) {
				break;
			}
		} else {
			excluded.push(found.id);
			find_list.push(found);
			total_time += found.duration;
			if (find_mode == find_song_shortest) {
				find_mode = find_song_closest;
				valid_lists.push({time: total_time, list: Array.from(find_list)});
			}
		}
		if (++iteration > max_iterations) {
			console.log("too many iterations", find_mode, excluded);
			valid_lists.push({time: total_time, list: Array.from(find_list)});
			break;
		}
	}
	var closest_valid_list = [];
	var closest_valid_time = 1/0;
	console.log(valid_lists);
	for (i in valid_lists) {
		var try_list = valid_lists[i].list;
		var try_time = valid_lists[i].time;
		if (Math.abs(try_time - time) < Math.abs(closest_valid_time - time)) {
			closest_valid_list = try_list;
			closest_valid_time = try_time;
		}
	}
	find_list = closest_valid_list;
	total_time = closest_valid_time;
	console.log(find_list, total_time, last_songs_timed);
	for (i in find_list) {
		last_songs_timed[last_songs_timed.length] = find_list[i].id;
	}
	if (last_songs_timed.length > find_list.length + 6) {
		last_songs_timed.splice(0, last_songs_timed.length - find_list.length - 6);
	}
	last_songs_duration = total_time;
	return find_list;
}

let find_song_longest = "longest";
let find_song_shortest = "shortest";
let find_song_closest = "closest";

function find_song(time, mode, exclude) {
	if (exclude == undefined) exclude = [];
	var best_song = null;
	l0: for (id in songs) {
		var meta = songs[id];
		for (x in exclude) {
			if (meta.id == exclude[x]) continue l0;
		}
		if (mode == find_song_longest) {
			if (meta.duration <= time && (!best_song || meta.duration > best_song.duration)) {
				best_song = meta;
			}
		} else if (mode == find_song_shortest) {
			if (meta.duration <= time && (!best_song || meta.duration < best_song.duration)) {
				best_song = meta;
			}
		} else if (mode == find_song_closest) {
			if (!best_song || Math.abs(meta.duration - time) < Math.abs(best_song.duration - time)) {
				best_song = meta;
			}
		}
	}
	return best_song;
}

function str_duration_to_seconds(duration) {
	var split = duration.split(":");
	var mult = [1, 60, 3600, 86400];
	var time = 0;
	for (i in split) {
		var num = Number.parseInt(split[i]);
		time += mult[split.length - i - 1] * num;
	}
	return time;
}

function clear_queue() {
	for (index in song_queue) {
		remove_from_queue(parseInt(index));
	}
}




