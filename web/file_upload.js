
var uploadId = 0;

var uploadStatus = {};
var uploadStarting = {};

// Initiates the file upload process.
function upload_file() {
	// Get file input element.
	var input = document.getElementById("file_uploader");
	var name  = document.getElementById("song_name_file").value;
	
	if (input.files.length) {
		// Perform an asynchronous read.
		reader = new FileReader()
		reader.onload = function () {
			uploadFileRead(name, new Uint8Array(this.result));
		}
		reader.readAsArrayBuffer(input.files[0]);
	}
}

// When a file is read, prepares for transmission.
function uploadFileRead(name, data) {
	// Get new upload ID.
	let id = uploadId ++;
	
	uploadStarting[id] = {
		// Local reference ID.
		localId: id,
		// Song ID.
		id:      null,
		// Data array.
		data:    data,
		// Amount of data sent.
		sent:    0,
		// Amount of data that needs uploading.
		size:    data.length
	};
	
	// Send request for uploading.
	// We will get a response with song ID and use that from then on.
	ws.send(JSON.stringify({upload_file_init: {name: name, ref: id, size: data.length}}));
}

// Handle upload progress messages.
function handle_upload_message(data) {
	if (data.status == "start") {
		uploadStatus[data.id] = uploadStarting[data.ref];
		uploadStatus[data.id].id = data.id;
		delete uploadStarting[data.ref];
		upload_data_bit(uploadStatus[data.id]);
	} else if (data.status == "continue") {
		upload_data_bit(uploadStatus[data.id]);
	} else if (data.status == "done") {
		delete uploadStatus[data.id];
	}
}

// Upload part of the file.
function upload_data_bit(status, count=64*1024) {
	// Clamp send count.
	if (count > status.size - status.sent) {
		count = status.size - status.sent;
	}
	
	// Grab part of array.
	var arr = [];
	for (var i = 0; i < count; i++) {
		arr[i] = status.data[i + status.sent];
	}
	status.sent += count;
	
	// Send data bit over.
	ws.send(JSON.stringify({upload_file_data: {id: status.id, data: arr}}));
}
