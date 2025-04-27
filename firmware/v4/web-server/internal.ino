/*

    WiFi Setup Functions

    To setup WiFi on your Web-stick
    put a file in your SD card with filename /cfg/wifi.txt
    In the text file, write two lines which containing the
    WiFi ssid and password, seperated by a linux new linx \n
    as follows:

    [WIFI SSID]
    [WiFi Password]
*/
String loadWiFiInfoFromSD() {
  if (SD.exists("/cfg/wifi.txt")) {
    String fileContent = "";
    File configFile = SD.open("/cfg/wifi.txt", FILE_READ);

    if (configFile) {
      while (configFile.available()) {
        fileContent += char(configFile.read());
      }
      configFile.close();
    }

    return fileContent;
  }

  return "";
}

void splitWiFiFileContent(const String& fileContent, String& ssid, String& password) {
  int newlineIndex = fileContent.indexOf('\n');
  if (newlineIndex != -1) {
    ssid = fileContent.substring(0, newlineIndex);
    password = fileContent.substring(newlineIndex + 1);
  }
}

void initWiFiConn() {
  String wifiFileContent = loadWiFiInfoFromSD();
  if (wifiFileContent.equals("")) {
    while (1) {
      Serial.println("WiFi info is not set. Please create a file in the SD card named /cfg/wifi.txt with first line as WiFi SSID and 2nd line as Password");
      delay(3000);
    }
  }

  // Split the contents into SSID and password
  String ssid, password;
  splitWiFiFileContent(wifiFileContent, ssid, password);
  ssid.trim();
  password.trim();
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi...");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(1000);
  }
  Serial.println(".");
  Serial.println("WiFi Connected: ");
  Serial.println("\n\nNetwork Configuration:");
  Serial.println("----------------------");
  Serial.print("         SSID: "); Serial.println(WiFi.SSID());
  Serial.print("  Wifi Status: "); Serial.println(WiFi.status());
  Serial.print("Wifi Strength: "); Serial.print(WiFi.RSSI()); Serial.println(" dBm");
  Serial.print("          MAC: "); Serial.println(WiFi.macAddress());
  Serial.print("           IP: "); Serial.println(WiFi.localIP());
  Serial.print("       Subnet: "); Serial.println(WiFi.subnetMask());
  Serial.print("      Gateway: "); Serial.println(WiFi.gatewayIP());
  Serial.print("        DNS 1: "); Serial.println(WiFi.dnsIP(0));
  Serial.print("        DNS 2: "); Serial.println(WiFi.dnsIP(1));
  Serial.print("        DNS 3: "); Serial.println(WiFi.dnsIP(2));
  Serial.println("----------------------");
  Serial.println();
}

/*

    Admin Credential Setup

    For management portal functions, you will
    need to setup the admin account in order to
    use it. Place a text file at cfg/admin.txt
    with the following information

    [admin_username]
    [admin_password]

    Currently only 1 admin user is supported

*/

String loadAdminCredFromSD() {
  if (SD.exists("/cfg/admin.txt")) {
    String fileContent = "";
    File configFile = SD.open("/cfg/admin.txt", FILE_READ);

    if (configFile) {
      while (configFile.available()) {
        fileContent += char(configFile.read());
      }
      configFile.close();
    }

    return fileContent;
  }

  return "";
}

void splitAdminCreds(const String& fileContent, String& username, String& password) {
  int newlineIndex = fileContent.indexOf('\n');
  if (newlineIndex != -1) {
    username = fileContent.substring(0, newlineIndex);
    password = fileContent.substring(newlineIndex + 1);
  }
}

void initAdminCredentials() {
  String adminCredentials = loadAdminCredFromSD();
  if (adminCredentials.equals("")) {
    //Disable authentications on API calls
    return;
  }

  // Split the contents into username and password
  splitWiFiFileContent(adminCredentials, adminUsername, adminPassword);
  adminUsername.trim();
  adminPassword.trim();
  Serial.println("Admin user loaded: " + adminUsername);
}

void initmDNSName() {
  if (SD.exists("/cfg/mdns.txt")) {
    String fileContent = "";
    File configFile = SD.open("/cfg/mdns.txt", FILE_READ);

    if (configFile) {
      while (configFile.available()) {
        fileContent += char(configFile.read());
      }
      configFile.close();
    }

    fileContent.trim();
    mdnsName = fileContent;
  }
}

//Load the previous login session key from database
//for resuming login session after poweroff
void initLoginSessionKey() {
  String serverCookie = DBRead("auth", "cookie");
  if (serverCookie != "") {
    authSession = serverCookie;
  }
}

/*

    MIME Utils

*/

String getMime(const String& path) {
  String _contentType = "text/plain";
  if (path.endsWith(".html")) _contentType = "text/html";
  else if (path.endsWith(".htm")) _contentType = "text/html";
  else if (path.endsWith(".css")) _contentType = "text/css";
  else if (path.endsWith(".json")) _contentType = "text/json";
  else if (path.endsWith(".js")) _contentType = "application/javascript";
  else if (path.endsWith(".png")) _contentType = "image/png";
  else if (path.endsWith(".gif")) _contentType = "image/gif";
  else if (path.endsWith(".jpg")) _contentType = "image/jpeg";
  else if (path.endsWith(".ico")) _contentType = "image/x-icon";
  else if (path.endsWith(".svg")) _contentType = "image/svg+xml";
  else if (path.endsWith(".eot")) _contentType = "font/eot";
  else if (path.endsWith(".woff")) _contentType = "font/woff";
  else if (path.endsWith(".woff2")) _contentType = "font/woff2";
  else if (path.endsWith(".ttf")) _contentType = "font/ttf";
  else if (path.endsWith(".xml")) _contentType = "text/xml";
  else if (path.endsWith(".pdf")) _contentType = "application/pdf";
  else if (path.endsWith(".zip")) _contentType = "application/zip";
  else if (path.endsWith(".gz")) _contentType = "application/x-gzip";
  else if (path.endsWith(".mp3")) _contentType = "audio/mpeg";
  else if (path.endsWith(".mp4")) _contentType = "video/mp4";
  else if (path.endsWith(".aac")) _contentType = "audio/aac";
  else if (path.endsWith(".ogg")) _contentType = "audio/ogg";
  else if (path.endsWith(".wav")) _contentType = "audio/wav";
  else if (path.endsWith(".m4v")) _contentType = "video/x-m4v";
  else if (path.endsWith(".webm")) _contentType = "video/webm";
  else _contentType = "text/plain";
  return _contentType;
}

/*

    Get ESP Info
*/

void printESPInfo() {
  Serial.println(ESP.getBootMode());
  Serial.print("ESP.getSdkVersion(); ");
  Serial.println(ESP.getSdkVersion());
  Serial.print("ESP.getBootVersion(); ");
  Serial.println(ESP.getBootVersion());
  Serial.print("ESP.getChipId(); ");
  Serial.println(ESP.getChipId());
  Serial.print("ESP.getFlashChipSize(); ");
  Serial.println(ESP.getFlashChipSize());
  Serial.print("ESP.getFlashChipRealSize(); ");
  Serial.println(ESP.getFlashChipRealSize());
  Serial.print("ESP.getFlashChipSizeByChipId(); ");
  Serial.println(ESP.getFlashChipSizeByChipId());
  Serial.print("ESP.getFlashChipId(); ");
  Serial.println(ESP.getFlashChipId());
}

/*

    Get SD card info

*/

uint32_t getTotalUsedSpace(const String& directory) {
  uint32_t totalUsedSpace = 0;

  File root = SD.open(directory);
  if (root) {
    while (true) {
      File entry = root.openNextFile();
      if (!entry) {
        // No more files
        break;
      }

      if (entry.isDirectory()) {
        // Recursive call for subdirectory
        totalUsedSpace += getTotalUsedSpace(directory + "/" + entry.name());
      } else {
        //Serial.print(entry.name());
        //Serial.print(" | Size: ");
        //Serial.println(humanReadableSize(entry.size()));
        totalUsedSpace += entry.size();
      }

      entry.close();
    }
    root.close();
  }

  return totalUsedSpace;
}

//Return the total used space on SD card
long getSDCardUsedSpace() {
  uint32_t totalUsedSpace = getTotalUsedSpace("/");
  Serial.println("Total used space: " + humanReadableSize(totalUsedSpace));
  return totalUsedSpace;
}

//Get the total SD card size
long getSDCardTotalSpace() {
  Serial.println("SD card size: " + humanReadableSize(SD.size64()));
  return SD.size64();
}

String humanReadableSize(const int bytes) {
  if (bytes < 1024) return String(bytes) + " B";
  else if (bytes < (1024 * 1024)) return String(bytes / 1024.0) + " KB";
  else if (bytes < (1024 * 1024 * 1024)) return String(bytes / 1024.0 / 1024.0) + " MB";
  else return String(bytes / 1024.0 / 1024.0 / 1024.0) + " GB";
}
