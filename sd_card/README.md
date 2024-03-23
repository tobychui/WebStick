# WebStick SD Card

This is the template for the web-stick SD card contents. Copy and paste these files into your freshly formatted FAT32 micro SD card for the web-stick to operate normally.

## Folder Structure

The folder structure of the webstick contains three root folders. 

- /cfg -> Folder for storing configuration files

- /www -> Folder for storing web files and contents (Except /www/store)

- /db -> Key-value database folder, auto create when system startup

#### Use as mp3 Streaming Site

If you are planning to store your music collections in WebStick, you can catergorize your music by folder. AirMusic embedded player do not provide music auto sorting or catergory function based on file metadata. The playlist is automatically generated from the folder contents.

#### Use as Tiny Cloud Storage

Inside the /www folder, you can put any kind of folders except the special path /www/store is reserve for private file storage. If the file is accessed via web server interface (i.e. directly open your browser to http://webstick.local/store ), a 401 Error will be shown.

To access the files inside /www/store, you can use the File Manager or file system API (/api/fs/download?file=/store/myfile.txt) if you are requesting such resources in your page.



## Pre-installed WebApps

There are a few pre-installed WebApps to get you started on your WebStick for your personal homepage with ease. 

- File Manager  (ArozOS File Manager)

- Markdown Editor (SimpleMDE)

- Notepad (Ace-Editor)

- Music Player (AirMusic)

- Photo Viewer

- Video Player (dPlayer)

- Search Tool

- System Info 

To start customizing your homepage, simply login to the admin panel and double click the html files in your root folder. You will be redirected to Notepad for realtime editing of your site.

## Configurations

#### WiFi Config

To config wifi, put your wifi SSID and password into cfg/wifi.txt.  The first line of the txt file is your wifi SSID and the 2nd line is the password. Make sure the new line is \n (Linux) instead of \r\n (Windows). Example as follows.

```
My Home WiFi
12345678
```

#### Admin Account

If you are using the API features, you can setup an admin account by creating a txt file named cfg/admin.txt, where the first line is the admin username and 2nd line is the password. Example as follows.

```
admin
password
```

#### mDNS

You can change the mDNS domain name by editing the mdns.txt file. By default, the name "webstick" is used and you can access your webstick using http://webstick.local if your browser support mDNS.



## Developers

### Backend API

If you want to develop custom apps for WebStick, you can make use of the following APIs defined in server.ino file. 

```arduino
 /* Authentication Functions */
  server.on("/api/auth/chk", HTTP_GET, HandleCheckAuth);
  server.on("/api/auth/login", HTTP_POST, HandleLogin);
  server.on("/api/auth/logout", HTTP_GET, HandleLogout);

  /* File System Functions */
  server.on("/api/fs/list", HTTP_GET, HandleListDir);
  server.on("/api/fs/del", HTTP_POST, HandleFileDel);
  server.on("/api/fs/move", HTTP_POST, HandleFileRename);
  server.on("/api/fs/download", HTTP_GET, HandleFileDownload);
  server.on("/api/fs/newFolder", HTTP_POST, HandleNewFolder);
  server.on("/api/fs/disk", HTTP_GET, HandleLoadSpaceInfo);
  server.on("/api/fs/properties", HTTP_GET, HandleFileProp);
  server.on("/api/fs/search", HTTP_GET, HandleFileSearch);
  
  /* Preference */
  server.on("/api/pref/set", HTTP_GET, HandleSetPref);
  server.on("/api/pref/get", HTTP_GET, HandleLoadPref);

  /* Others */
  server.on("/api/info/wifi", HTTP_GET, HandleWiFiInfo);
```

Note that for POST paramter, please put them in the URL paramter (e.g. /api/fs/search?keyword=.mp3) instead of POST body, as ESP8266 do not have enough memory to parse the body.



### Usage Examples

Here are some usage examples of the API call

```javascript
"/api/auth/login?username=" + username + "&password=" + password
"/api/fs/list?dir=" + path
"/api/fs/del?target=" + path
"/api/fs/move?src=" + path + "&dest=" + newpath + filename
"/api/fs/download?preview=true&file=" + filepath
"/api/fs/properties?file=" + filepath
"/api/fs/search?keyword=" + keyword

//Disk Info API Example
$.get("/api/fs/disk", function(data){
	if (data.error != undefined){
		alert(data.error);
		return;
	}
	let usedPercentage = data.usedSpace / data.diskSpace * 100;
	console.log(usedPercentage + "%");
});

//Set and Get Preference
$.ajax({
	type: 'GET',
	url: "/api/pref/set",
	data: {key: "myApp/" + configName, value:configValue},
	success: function(data){},


});

$.ajax({
	type: 'GET',
	url: "/api/pref/get",
	data: {key: "myApp/" + configName},
	success: function(data){
			if (data.error !== undefined){
				//handle error
			}else{
				console.log(data);
			}
		},
	error: function(){console.log("error");},
	timeout: 3000
});
```

**Note that preference set require logged in user while preference get do not. So you can make use of this properties and use preference API for rendering on things like blog post or guest READ ONLY materials on your site.**

### File Upload

The endpoint /upload can be used to upload file using multipart FORM data. See /www/admin/upld.js for an example implementation, or you can directly include upld.js in your page and call to handleFile like the example below.

```javascript
const blob = new Blob(["Hello World!"], { type: 'text/plain' });
const file = new File([blob], "hello.txt");
handleFile(file, "/down/", function(){
    alert("Upload done");
});
```

The upload function also act as the "file write" function for the WebStick backend infrastructure.

## Limitations

The implementation already used AsyncTCP for best performance. Due to the limitation of hardware, the system have straight limitations on what file can be uploaded or downloaded.

- Filepath must be 255 characters or less. Note that it is filepath length not filename, and UTF-8 characters will occupy from 1 - 4 bytes.  By default, the backend limits the filename length to 32 chars (extension included)

- Filesize of upload, although not limited, based on our testing, it is recommended to use files <= 5MB in size, including image, music or script files.

- Use CDN if possible to reduce CPU load on the MCU



## Too slow?

This project is a trim-down version of the full feature [ArozOS](https://arozos.com) Open Source Project. If you want to host your own budget cloud system with even more features, visit [https://arozos.com](https://arozos.com) for more information.
