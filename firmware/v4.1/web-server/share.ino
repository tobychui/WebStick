/*
    Share.ino

    This module handle file sharing on the WebSticks
    Recommended file size <= 5MB

*/

//Create a file share, must be logged in
void HandleCreateShare(AsyncWebServerRequest *r) {
  if (!HandleAuth(r)) {
    return;
  }

  //Get filename from parameters
  String filepath = GetPara(r, "filename");
  filepath.trim();
  //filepath is the subpath under the www folder
  // e.g. "/www/myfile.txt" filepath will be "/myfile.txt"
  if (filepath == "") {
    SendErrorResp(r, "invalid filename given");
    return;
  }

  if (IsFileShared(filepath)) {
    SendErrorResp(r, "target file already shared");
    return;
  }

  //Add a share entry for this file
  String shareID = GeneratedRandomHex();
  bool succ = DBWrite("shln", shareID, filepath);
  if (!succ) {
    SendErrorResp(r, "unable to save share entry");
    return;
  }
  succ = DBWrite("shfn", filepath, shareID);
  if (!succ) {
    SendErrorResp(r, "unable to save share entry");
    return;
  }

  Serial.println("Shared: " + filepath + " with ID: " + shareID);
  SendOK(r);
}

//Remove a file share
void HandleRemoveShare(AsyncWebServerRequest *r) {
  if (!HandleAuth(r)) {
    return;
  }

  //Get filename from parameters
  String filepath = GetPara(r, "filename");
  filepath.trim();
  if (filepath == "") {
    SendErrorResp(r, "invalid filename given");
    return;
  }

  if (!IsFileShared(filepath)) {
    SendErrorResp(r, "target file is not shared");
    return;
  }

  //Get the share ID of this entry
  String shareId = DBRead("shfn", filepath);
  if (shareId == "") {
    SendErrorResp(r, "unable to load share entry");
    return;
  }

  //Remove share entry in both tables
  bool succ = DBRemove("shln", shareId);
  if (!succ) {
    SendErrorResp(r, "unable to remove share entry");
    return;
  }
  succ = DBRemove("shfn", filepath);
  if (!succ) {
    SendErrorResp(r, "unable to remove share entry");
    return;
  }

  Serial.println("Removed shared file " + filepath + " (share ID: " + shareId + ")");
  SendOK(r);
}

//List all shared files
void HandleShareList(AsyncWebServerRequest *r) {
  if (!HandleAuth(r)) {
    return;
  }

  //Build the json with brute force
  String jsonString = "[";
  //As the DB do not support list, it directly access the root of the folder where the kvdb stores the entries
  File root = SD.open(DB_root + "shln/");
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

        //Filter out all the directory if any
        if (entry.isDirectory()) {
          continue;
        }

        //Read the filename from file
        String filename = "";
        while (entry.available()) {
          filename = filename + entry.readString();
        }

        //Append to the JSON line
        jsonString = jsonString + "{\"filename\":\"" + basename(filename) + "\", \"filepath\":\"" + filename + "\", \"shareid\":\"" + entry.name() + "\"}";
      }
    }
  }
  jsonString += "]";

  r->send(200, "application/json", jsonString);
}

//Serve a shared file, do not require login
void HandleShareAccess(AsyncWebServerRequest *r) {
  String shareID = GetPara(r, "id");
  if (shareID == "") {
    r->send(404, "text/plain", "Not Found");
    return;
  }

  //Download request
  String sharedFilename = GetFilenameFromShareID(shareID);
  if (sharedFilename == "") {
    r->send(404, "text/plain", "Share not found");
    return;
  }

  //Check if the file still exists on SD card
  String realFilepath = "/www" + sharedFilename;
  File targetFile = SD.open(realFilepath);
  if (!targetFile) {
    r->send(404, "text/plain", "Shared file no longer exists");
    return;;
  }


  if (r->hasParam("download")) {
    //Serve the file
    r->send(SDFS, realFilepath, getMime(sharedFilename), false);
  } else if (r->hasParam("prop")) {
    //Serve the file properties
    File targetFile = SD.open(realFilepath);
    if (!targetFile) {
      SendErrorResp(r, "File open failed");
      return;
    }

    String resp = "{\"filename\":\"" + basename(sharedFilename) + "\",\"filepath\":\"" + sharedFilename + "\",\"isDir\":false,\"filesize\":" + String(targetFile.size()) + ",\"shareid\":\"" + shareID + "\"}";
    targetFile.close();
    SendJsonResp(r, resp);
  } else {
    //serve download UI template
    r->send(SDFS, "/www/admin/share.html", "text/html", false);
    return;
  }
}

//Clear the shares that no longer exists
void HandleShareListCleaning(AsyncWebServerRequest *r) {
  if (!HandleAuth(r)) {
    return;
  }

  File root = SD.open(DB_root + "shln/");
  bool firstObject = true;
  if (root) {
    while (true) {
      File entry = root.openNextFile();
      if (!entry) {
        // No more files
        break;
      } else {
        //Filter out all the directory if any
        if (entry.isDirectory()) {
          continue;
        }

        //Read the filename from file
        String filename = "";
        while (entry.available()) {
          filename = filename + entry.readString();
        }

        //Check if the target file still exists
        File targetFile = SD.open("/www" + filename);
        if (!targetFile) {
          //File no longer exists. Remove this share entry
          DBRemove("shln", entry.name());
          DBRemove("shfn", filename);
        } else {
          //File still exists.
          targetFile.close();
        }
      }
    }
  }

  SendOK(r);
}


//Get the file share ID from filename, return empty string if not shared
String GetFileShareIDByFilename(String filepath) {
  return DBRead("shfn", filepath);
}

//Get the filename (without /www prefix) from share id
// return empty string if not found
String GetFilenameFromShareID(String shareid) {
  return DBRead("shln", shareid);
}


//Check if a file is shared
bool IsFileShared(String filepath) {
  //Check if the file is shared
  return DBKeyExists("shfn", filepath);
}
