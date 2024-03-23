/*

    Web Server

    This is the main entry point of the WebStick bare metal
    web server. If you have exception rules that shall not
    be handled by the main router, you can do them here.

*/

//Check if a user is authenticated / logged in
bool IsUserAuthed(AsyncWebServerRequest *request) {
  if (request->hasHeader("Cookie")) {
    //User cookie from browser
    String authCookie = GetCookieValueByKey(request, "web-auth");
    if (authCookie == "") {
      return false;
    }

    //Match it to the server side value in kvdb
    //Serial.println(authCookie); //user cookie
    //Serial.println(authSession); //server session
    if (authSession == "") {
      //Server side has no resumable login session
      return false;
    }
    if (authCookie.equals(authSession)) {
      return true;
    }
    return false;
  } else {
    Serial.println("Cookie Missing");
    return false;
  }
}

//Reply the request by a directory list
void HandleDirRender(AsyncWebServerRequest *r, String dirName, String dirToList) {
  AsyncResponseStream *response = r->beginResponseStream("text/html");
  //Serve directory entries
  File directory = SD.open(dirToList);

  // Check if the directory is open
  if (!directory) {
    SendErrorResp(r, "unable to open directory");
    return;
  }

  response->print("<!DOCTYPE html><html><head><meta charset=\"utf-8\"><title>Content of " + dirName + "</title></head><body style=\"margin: 3em;font-family: Arial;\">");
  response->print("<h3>Content of " + dirName + "</h3><div style=\"width: 100%;border-bottom: 1px solid #d9d9d9;\"></div><ul>");
  // List the contents of the directory
  while (true) {
    File entry = directory.openNextFile();
    if (!entry) {
      // No more files
      break;
    }

    // Print the file name
    response->print("<li><a href=\"./" + String(entry.name()) + "\">");
    response->print(entry.name());
    response->print(" (" + humanReadableSize(entry.size()) + ")</a></li>");
    Serial.println(entry.name());
    
    entry.close();
  }

  // Close the directory
  directory.close();
  
  response->print("</ul><div style=\"width: 100%;border-bottom: 1px solid #d9d9d9;\"></div><br><a href=\"../\">Back</a>");
  response->print("<br><br><body></html>");
  r->send(response);
}

void initWebServer() {
  /*
     Other handles here, like this
    server.on("/logout", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(401);
    });
  */

  /*
  server.on("/test", HTTP_GET, [](AsyncWebServerRequest * request) {
    getSDCardUsedSpace();
    request->send(200);
  });
  */

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

  //File upload handler. see upload.ino
  server.onFileUpload(handleFileUpload);

  //Not found handler
  server.onNotFound([](AsyncWebServerRequest * request) {
    //Generally it will not arrive here as NOT FOUND is also handled in the main router.
    //See router.ino for implementation details.
    prettyPrintRequest(request);
    request->send(404, "text/plain", "Not Found");
  });

  //Main Router, see router.ino
  server.addHandler(new MainRouter());

  server.begin();
}
