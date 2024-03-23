/*

    API.ino

    This script handle API requests
    functions.
*/

/* Utilities Functions */
String GetPara(AsyncWebServerRequest * request, String key) {
  if (request->hasParam(key)) {
    return request->getParam(key)->value();
  }
  return "";
}

void SendErrorResp(AsyncWebServerRequest * r, String errorMessage) {
  //Parse the error message into json
  StaticJsonDocument<200> jsonDocument;
  JsonObject root = jsonDocument.to<JsonObject>();
  root["error"] = errorMessage;
  String jsonString;
  serializeJson(root, jsonString);

  //Send it out with request handler
  r->send(200, "application/json", jsonString);
}

void SendJsonResp(AsyncWebServerRequest * r, String jsonString) {
  r->send(200, "application/json", jsonString);
}

void SendOK(AsyncWebServerRequest *r) {
  r->send(200, "application/json", "\"ok\"");
}

//Handle auth check if the request has been authed.
//Return false if user is not authed
bool HandleAuth(AsyncWebServerRequest *request) {
  //Handle for API calls authentication validate
  if (!IsUserAuthed(request)) {
    //user not logged in
    request->send(401, "text/html", "401 - Unauthorized");
    return false;
  }

  return true;
}

/*

    Handler Functions

    These are API endpoints handler
    which handle special API call
    for backend operations

*/

/* Authentications */
//Check if the user has logged in
void HandleCheckAuth(AsyncWebServerRequest *r) {
  if (IsUserAuthed(r)) {
    SendJsonResp(r, "true");
  } else {
    SendJsonResp(r, "false");
  }
}

//Handle login request
void HandleLogin(AsyncWebServerRequest *r) {
  String username = GetPara(r, "username");
  String password = GetPara(r, "password");
  if (adminUsername == "") {
    SendErrorResp(r, "admin account not enabled");
    return;
  }
  if (username.equals(adminUsername) && password.equals(adminPassword)) {
    //Username and password correct. Set a cookie for this login.
    //Generate a unique cookie for this login session
    String cookieId = GeneratedRandomHex();
    Serial.print("Generating new cookie ID ");
    Serial.println(cookieId);

    String expireUTC = getUTCTimeString(getTime() + 604800);
    Serial.print("Generating expire UTC timestamp ");
    Serial.println(expireUTC);

    AsyncWebServerResponse *response = r->beginResponse(200, "application/json", "\"ok\"");
    response->addHeader("Server", mdnsName);
    response->addHeader("Cache-Control", "no-cache");
    response->addHeader("Set-Cookie",  "web-auth=" + cookieId + "; Path=/; Expires=" + expireUTC + "; Max-Age=604800");

    //Save the cookie id
    DBWrite("auth", "cookie", cookieId);
    authSession = cookieId;

    //Return login succ
    r->send(response);

    Serial.println(username + " logged in");
  } else {
    SendErrorResp(r, "invalid username or password");
    return;
  }
  SendOK(r);
}

//Handle logout request, or you can logout with
//just front-end by going to log:out@{ip_addr}/api/auth/logout
void HandleLogout(AsyncWebServerRequest *r) {
  if (!IsUserAuthed(r)) {
    SendErrorResp(r, "not logged in");
    return;
  }

  //Remove the cookie on client side
  AsyncWebServerResponse *response = r->beginResponse(200, "application/json", "\"ok\"");
  response->addHeader("Server", mdnsName);
  response->addHeader("Cache-Control", "no-cache");
  response->addHeader("Set-Cookie",  "web-auth=deleted; path=/; expires=Thu, 01 Jan 1970 00:00:00 GMT");
  r->send(response);

  //Delete the server side cookie
  DBRemove("auth", "cookie");
  authSession = "";
}

/* File System Functions */
//HandleListDir handle the listing of directory under /www/
void HandleListDir(AsyncWebServerRequest *r) {
  if (!HandleAuth(r)) {
    return;
  }

  //Get the folder path to be listed
  //As ESP8266 dont have enough memory for proper struct to json conv, we are hacking a json string out of a single for-loop
  String jsonString = "[";
  String folderPath = GetPara(r, "dir");
  folderPath = "/www" + folderPath;
  if (SD.exists(folderPath)) {
    File root = SD.open(folderPath);
    bool firstObject = true;
    if (root) {
      while (true) {
        File entry = root.openNextFile();
        if (!entry) {
          // No more files
          break;
        } else {
          //There are more lines. Add a , to the end of the previous json object
          if (!firstObject) {
            jsonString = jsonString + ",";
          } else {
            firstObject = false;
          }

        }

        String isDirString = "true";
        if (!entry.isDirectory()) {
          isDirString = "false";
        }
        jsonString = jsonString + "{\"Filename\":\"" + entry.name() + "\",\"Filesize\":" + String(entry.size()) + ",\"IsDir\":" + isDirString + "}";
        entry.close();
      }
      root.close();

      jsonString += "]";
      SendJsonResp(r, jsonString);
    } else {
      SendErrorResp(r, "path read failed");
    }
  } else {
    SendErrorResp(r, "path not found");
  }
  Serial.println(folderPath);
}

//Handle file rename on SD card
void HandleFileRename(AsyncWebServerRequest *r) {
  if (!HandleAuth(r)) {
    return;
  }
  String srcFile = GetPara(r, "src");
  String destFile = GetPara(r, "dest");
  srcFile.trim();
  destFile.trim();
  srcFile = "/www" + srcFile;
  destFile = "/www" + destFile;
  if (!SD.exists(srcFile)) {
    SendErrorResp(r, "source file not exists");
    return;
  }

  if (SD.exists(destFile)) {
    SendErrorResp(r, "destination file already exists");
    return;
  }

  SD.rename(srcFile, destFile);
  SendOK(r);
}


//Handle file delete on SD card
void HandleFileDel(AsyncWebServerRequest *r) {
  if (!HandleAuth(r)) {
    return;
  }
  String targetFile = GetPara(r, "target");
  targetFile.trim();
  if (targetFile.equals("/")) {
    //Do not allow remove the whole root folder
    SendErrorResp(r, "you cannot remove the root folder");
    return;
  }
  targetFile = "/www" + targetFile;
  if (!SD.exists(targetFile)) {
    SendErrorResp(r, "target file not exists");
    return;
  }



  if (!SD.remove(targetFile)) {
    if (IsDir(targetFile)) {
      if (!targetFile.endsWith("/")) {
        //pad the tailing slash
        targetFile = targetFile + "/";
      }

      //Try remove dir
      if (!SD.rmdir(targetFile)) {
        //The folder might contain stuffs. Do recursive delete
        Serial.println("rmdir failed. Trying recursive delete");
        if (recursiveDirRemove(targetFile)) {
          SendOK(r);
        } else {
          SendErrorResp(r, "folder delete failed");
        }

      } else {
        SendOK(r);
      }
    } else {
      SendErrorResp(r, "file delete failed");
    }
    return;
  }

  SendOK(r);
}

//Hanle creating new Folder
void HandleNewFolder(AsyncWebServerRequest *r) {
  if (!HandleAuth(r)) {
    return;
  }

  //Get and clean the folder path
  String folderPath = GetPara(r, "path");
  folderPath.trim();
  if (folderPath == "") {
    SendErrorResp(r, "invalid folder path given");
    return;
  }

  folderPath = "/www" + folderPath;

  if (SD.exists(folderPath)) {
    //Already exists
    SendErrorResp(r, "folder with given path already exists");
    return;
  }

  SD.mkdir(folderPath);
  SendOK(r);
}

//Handle download file from any path, including the private store folder
void HandleFileDownload(AsyncWebServerRequest *r) {
  if (!HandleAuth(r)) {
    return;
  }
  String targetFile = GetPara(r, "file");
  String preview = GetPara(r, "preview");
  if (!SD.exists("/www" + targetFile)) {
    r->send(404, "text/html", "404 - File Not Found");
    return;
  }

  //Check if it is dir, ESP8266 have no power of creating zip file
  if (IsDir("/www" + targetFile)) {
    r->send(500, "text/html", "500 - Internal Server Error: Target is a folder");
    return;
  }

  //Ok. Proceed with file serving
  if (preview == "true") {
    //Serve
    r->send(SDFS, "/www" + targetFile, getMime(targetFile), false);
  } else {
    //Download
    r->send(SDFS, "/www" + targetFile, "application/octet-stream", false);
  }

}

//Get the file / folder properties
void HandleFileProp(AsyncWebServerRequest *r) {
  if (!HandleAuth(r)) {
    return;
  }
  String filepath = GetPara(r, "file");
  filepath.trim();
  String realFilepath = "/www" + filepath;

  String resp = "";
  if (IsDir(realFilepath)) {
    //target is a folder
    uint32_t totalSize = 0;
    uint16_t fileCount = 0;
    uint16_t folderCount = 0;
    analyzeDirectory(realFilepath, totalSize, fileCount, folderCount);
    resp = "{\"filename\":\"" + basename(filepath) + "\",\"filepath\":\"" + filepath + "\",\"isDir\":true,\"filesize\":" + String(totalSize) + ",\"fileCounts\":" + String(fileCount) + ",\"folderCounts\":" + String(folderCount) + "}";

  } else {
    //target is a file
    File targetFile = SD.open(realFilepath);
    if (!targetFile) {
      SendErrorResp(r, "File open failed");
      return;
    }

    resp = "{\"filename\":\"" + basename(filepath) + "\",\"filepath\":\"" + filepath + "\",\"isDir\":false,\"filesize\":" + String(targetFile.size()) + "}";
    targetFile.close();
  }

  SendJsonResp(r, resp);
}

//Search with basic keyword match
void HandleFileSearch(AsyncWebServerRequest *r) {
  if (!HandleAuth(r)) {
    return;
  }
  String keyword = GetPara(r, "keyword");
  keyword.trim();
  if (keyword.equals("")) {
    SendErrorResp(r, "keyword cannot be empty");
    return;
  }

  //Prepare for the resp
  AsyncResponseStream *response = r->beginResponseStream("application/json");
  response->print("[");

  //Recursive search for the whole /www/ directory (Require tailing slash)
  int foundCounter = 0;
  scanSDCardForKeyword("/www/", keyword, &foundCounter, response);

  //Send the resp
  response->print("]");
  r->send(response);
}
/* Handle Load and Set of Prefernece Value */
//Set Preference, auth user only
void HandleSetPref(AsyncWebServerRequest *r) {
  if (!HandleAuth(r)) {
    return;
  }
  String key = GetPara(r, "key");
  String val = GetPara(r, "value");
  key.trim();
  val.trim();

  if (key == "" || val == "") {
    SendErrorResp(r, "invalid key or value given");
    return;
  }

  DBWrite("pref", key, val);
  SendOK(r);
}

//Load Prefernece, allow public access
void HandleLoadPref(AsyncWebServerRequest *r) {
  String key = GetPara(r, "key");
  key.trim();
  if (!DBKeyExists("pref", key)) {
    SendErrorResp(r, "preference with given key not found");
    return;
  }

  String prefValue = DBRead("pref", key);
  r->send(200, "application/json", "\"" + prefValue + "\"");
}

/* Handle System Info */
void HandleLoadSpaceInfo(AsyncWebServerRequest *r) {
  String jsonResp = "{\
    \"diskSpace\":" + String(getSDCardTotalSpace()) + ",\
    \"usedSpace\": " + String(getSDCardUsedSpace()) + "\
  }";

  SendJsonResp(r, jsonResp);
}

//Get the current connected WiFi info
void HandleWiFiInfo(AsyncWebServerRequest *r) {
  StaticJsonDocument<256> jsonBuffer;
  jsonBuffer["SSID"] = WiFi.SSID();
  jsonBuffer["WifiStatus"] = WiFi.status();
  jsonBuffer["WifiStrength"] = WiFi.RSSI();
  jsonBuffer["MAC"] = WiFi.macAddress();
  jsonBuffer["IP"] = WiFi.localIP().toString();
  jsonBuffer["Subnet"] = WiFi.subnetMask().toString();
  jsonBuffer["Gateway"] = WiFi.gatewayIP().toString();
  jsonBuffer["DNS1"] = WiFi.dnsIP(0).toString();
  jsonBuffer["DNS2"] = WiFi.dnsIP(1).toString();
  jsonBuffer["DNS3"] = WiFi.dnsIP(2).toString();

  // Serialize the JSON buffer to a string
  String jsonString;
  serializeJson(jsonBuffer, jsonString);
  SendJsonResp(r, jsonString);
}
