/*
  User.ino

  This is a new module handling user systems on ESP8266

*/

//Check if a user login is valid by username and password
bool UserCheckAuth(String username, String password) {
  username.trim();
  password.trim();
  //Load user info from db
  if (!DBKeyExists("user", username)) {
    return false;
  }

  String userHashedPassword = DBRead("user", username);  //User hashed password from kvdb
  String enteredHashedPassword = sha1(password);         //Entered hashed password
  return userHashedPassword.equals(enteredHashedPassword);
}

//Get the username from the request, return empty string if unable to resolve
String GetUsernameFromRequest(AsyncWebServerRequest *r) {
  if (r->hasHeader("Cookie")) {
    //User cookie from browser
    String authCookie = GetCookieValueByKey(r, "web-auth");
    if (authCookie == "") {
      return "";
    }

    //Check if this is admin login
    if (authCookie.equals(authSession)) {
      return adminUsername;
    }

    //Check if user login
    if (DBKeyExists("sess", authCookie)) {
      //Return the username of this session
      String username = DBRead("sess", authCookie);
      return username;
    }else{
      Serial.println("session cookie not found: " + authCookie);
    }

    //Not found
    return "";
  }

  //This user have no cookie in header
  return "";
}

//Create new user, creator must be admin
void HandleNewUser(AsyncWebServerRequest *r) {
  if (!IsAdmin(r)) {
    SendErrorResp(r, "this function require admin permission");
    return;
  }
  String username = GetPara(r, "username");
  String password = GetPara(r, "password");
  username.trim();
  password.trim();

  //Check if the inputs are valid
  if (username == "" || password == "") {
    SendErrorResp(r, "username or password is an empty string");
    return;
  } else if (password.length() < 8) {
    SendErrorResp(r, "password must contain at least 8 characters");
    return;
  }

  //Check if the user already exists
  if (DBKeyExists("user", username)) {
    SendErrorResp(r, "user with name: " + username + " already exists");
    return;
  }

  //OK create the user
  bool succ = DBWrite("user", username, sha1(password));
  if (!succ) {
    SendErrorResp(r, "write new user to database failed");
    return;
  }
  r->send(200, "application/json", "\"OK\"");
}

//Remove the given username from the system
void HandleRemoveUser(AsyncWebServerRequest *r) {
  if (!IsAdmin(r)) {
    SendErrorResp(r, "this function require admin permission");
    return;
  }

  String username = GetPara(r, "username");
  username.trim();

  //Check if the user exists
  if (!DBKeyExists("user", username)) {
    SendErrorResp(r, "user with name: " + username + " not exists");
    return;
  }

  //Okey, remove the user
  bool succ = DBRemove("user", username);
  if (!succ) {
    SendErrorResp(r, "remove user from system failed");
    return;
  }
  r->send(200, "application/json", "\"OK\"");
}

//Admin or the user themselve change password for the account
void HandleUserChangePassword(AsyncWebServerRequest *r) {
  //Get requesting username
  if (!IsUserAuthed(r)) {
    SendErrorResp(r, "user not logged in");
    return;
  }

  String currentUser = GetUsernameFromRequest(r);
  if (currentUser == "") {
    SendErrorResp(r, "unable to load user from system");
    return;
  }

  //Check if the user can change password
  //note that admin password cannot be changed on-the-fly
  //admin password can only be changed in SD card config file
  String modifyingUsername = GetPara(r, "username");
  String newPassword = GetPara(r, "newpw");
  modifyingUsername.trim();
  newPassword.trim();

  if (modifyingUsername == adminUsername) {
    SendErrorResp(r, "admin username can only be changed in the config file");
    return;
  }
  if (currentUser == adminUsername || modifyingUsername == currentUser) {
    //Allow modify
    if (newPassword.length() < 8) {
      SendErrorResp(r, "password must contain at least 8 characters");
      return;
    }

    //Write to database
    bool succ = DBWrite("user", modifyingUsername, sha1(newPassword));
    if (!succ) {
      SendErrorResp(r, "write new user to database failed");
      return;
    }
    SendOK(r);

  } else {
    SendErrorResp(r, "permission denied");
    return;
  }

  SendOK(r);
}

//Get the current username
void HandleGetUserinfo(AsyncWebServerRequest *r){
  if (!HandleAuth(r)) {
    return;
  }

  String isAdmin = "false";
  if (IsAdmin(r)){
    isAdmin = "true";
  }

  String username = GetUsernameFromRequest(r);
  r->send(200, "application/json", "{\"username\":\"" + username + "\", \"admin\":" + isAdmin + "}");
}

//List all users registered in this WebStick
void HandleUserList(AsyncWebServerRequest *r) {
  if (!HandleAuth(r)) {
    return;
  }

  //Build the json with brute force
  String jsonString = "[";
  //As the DB do not support list, it directly access the root of the folder where the kvdb stores the entries
  File root = SD.open(DB_root + "user/");
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

        //Append to the JSON line
        jsonString = jsonString + "{\"Username\":\"" + entry.name() + "\"}";
      }
    }
  }
  jsonString += "]";

  r->send(200, "application/json", jsonString);
  
}
