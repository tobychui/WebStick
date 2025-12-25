/*

    Utilities

*/

//Pretty print for easy debug
void prettyPrintRequest(AsyncWebServerRequest * request) {
  if (request->method() == HTTP_GET)
    Serial.printf("GET");
  else if (request->method() == HTTP_POST)
    Serial.printf("POST");
  else if (request->method() == HTTP_DELETE)
    Serial.printf("DELETE");
  else if (request->method() == HTTP_PUT)
    Serial.printf("PUT");
  else if (request->method() == HTTP_PATCH)
    Serial.printf("PATCH");
  else if (request->method() == HTTP_HEAD)
    Serial.printf("HEAD");
  else if (request->method() == HTTP_OPTIONS)
    Serial.printf("OPTIONS");
  else
    Serial.printf("UNKNOWN");
  Serial.printf(" http://%s%s\n", request->host().c_str(), request->url().c_str());

  if (request->contentLength()) {
    Serial.printf("_CONTENT_TYPE: %s\n", request->contentType().c_str());
    Serial.printf("_CONTENT_LENGTH: %u\n", request->contentLength());
  }

  int headers = request->headers();
  int i;
  for (i = 0; i < headers; i++) {
    const AsyncWebHeader* h = request->getHeader(i);
    Serial.printf("_HEADER[%s]: %s\n", h->name().c_str(), h->value().c_str());
  }

  int params = request->params();
  for (i = 0; i < params; i++) {
    const AsyncWebParameter* p = request->getParam(i);
    if (p->isFile()) {
      Serial.printf("_FILE[%s]: %s, size: %u\n", p->name().c_str(), p->value().c_str(), p->size());
    } else if (p->isPost()) {
      Serial.printf("_POST[%s]: %s\n", p->name().c_str(), p->value().c_str());
    } else {
      Serial.printf("_GET[%s]: %s\n", p->name().c_str(), p->value().c_str());
    }
  }
}

//Check if a path is dir
bool IsDir(const String& path) {
  File file = SD.open(path);
  if (file && !file.isDirectory()) {
    file.close();
    return false;
  }
  file.close();
  return true;
}

//Generate a random 32 bit hex
String GeneratedRandomHex() {
  String hexString = "";

  for (int i = 0; i < 8; i++) {
    byte randomValue = random(256);  // Generate a random byte (0 to 255)
    hexString += String(randomValue, HEX);  // Convert the random byte to its hexadecimal representation
  }

  return hexString;
}

//Get the current unix timestamp
unsigned long getTime() {
  unsigned long now = timeClient.getEpochTime();
  return now;
}

//Convert Unix timestamp to UTC string
String getUTCTimeString(time_t unixTimestamp) {
  char utcString[30];
  const char* weekdayNames[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
  const char* monthNames[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

  tm* timeInfo = gmtime(&unixTimestamp);
  sprintf(utcString, "%s, %02d %s %04d %02d:%02d:%02d GMT",
          weekdayNames[timeInfo->tm_wday], timeInfo->tm_mday, monthNames[timeInfo->tm_mon],
          timeInfo->tm_year + 1900, timeInfo->tm_hour, timeInfo->tm_min, timeInfo->tm_sec);

  return String(utcString);
}

//Get a certain cookie value by its key
String GetCookieValueByKey(AsyncWebServerRequest *request, const String& key) {
  //Check if the Cookie header exists
  if (!request->hasHeader("Cookie")) {
    return "";
  }

  //Load the cookie value as string from header
  String targetValue = "";
  const AsyncWebHeader* h = request->getHeader("Cookie");
  String cookieHeader = String(h->value().c_str());

  // Find the start and end positions of target cookie key
  int startIndex = cookieHeader.indexOf(key + "=");
  if (startIndex != -1) {
    startIndex += 9;  // Length of "web-auth="

    int endIndex = cookieHeader.indexOf(';', startIndex);
    if (endIndex == -1) {
      endIndex = cookieHeader.length();
    }

    // Extract the value of the "web-auth" cookie
    targetValue = cookieHeader.substring(startIndex, endIndex);
  }

  return targetValue;
}

//Get the filename from filepath
String basename(const String& filePath) {
  int lastSlashIndex = filePath.lastIndexOf('/');

  // If no slash is found, return the original path
  if (lastSlashIndex == -1) {
    return filePath;
  }

  // Return the substring after the last slash
  return filePath.substring(lastSlashIndex + 1);
}


bool recursiveDirRemove(const String& path) {
  Serial.println(path);
  File directory = SD.open(path);
  if (!directory) {
    Serial.println("Error opening directory!");
    return false;
  }

  // Delete all the files in the directory
  directory.rewindDirectory();
  while (true) {
    File entry = directory.openNextFile();
    if (!entry) {
      // No more files
      break;
    }

    String filename = String(entry.name());
    if (entry.isDirectory()) {
      // Recursively delete the subdirectory
      recursiveDirRemove(path + filename);
    } else {
      // Delete the file
      entry.close();
      Serial.println("Removing " + path + filename);
      SD.remove(path + filename);
    }
  }

  // Close the directory
  directory.close();

  // Delete the directory itself
  if (!SD.rmdir(path)) {
    Serial.println("Error deleting directory!");
    return false;
  }

  return true;
}

/*

   uint32_t totalSize = 0;
   uint16_t fileCount = 0;
   uint16_t folderCount = 0;

   // Call the recursive function to analyze the directory and its contents
   analyzeDirectory(directoryPath, totalSize, fileCount, folderCount);
*/
void analyzeDirectory(const String& path, uint32_t& totalSize, uint16_t& fileCount, uint16_t& folderCount) {
  File directory = SD.open(path);
  if (!directory) {
    Serial.println("Error opening directory!");
    return;
  }

  // Analyze all files and subdirectories in the directory
  directory.rewindDirectory();
  while (true) {
    File entry = directory.openNextFile();
    if (!entry) {
      // No more files
      break;
    }

    if (entry.isDirectory()) {
      // Recursively analyze the subdirectory
      folderCount++;
      analyzeDirectory(entry.name(), totalSize, fileCount, folderCount);
    } else {
      // Update the counters and add file size to total size
      fileCount++;
      totalSize += entry.size();
    }

    entry.close();
  }

  // Close the directory
  directory.close();
}

void scanSDCardForKeyword(const String& directoryPath, const String& keyword, int *matchCounter, AsyncResponseStream *response) {
  File directory = SD.open(directoryPath);
  if (!directory) {
    Serial.println("Error opening directory " + directoryPath);
    return;
  }

  // Scan all files and subdirectories in the directory
  directory.rewindDirectory();
  while (true) {
    File entry = directory.openNextFile();
    if (!entry) {
      // No more files
      break;
    }

    if (entry.isDirectory()) {
      // Recursively scan the subdirectory
      scanSDCardForKeyword((directoryPath + entry.name() + "/"), keyword, matchCounter, response);
    } else {
      // Check if the filename contains the keyword
      String filename = basename(entry.name());
      if (filename.indexOf(keyword) != -1) {
        if ((*matchCounter) > 0){
          //Append a comma before appending next matching file
          response->print(",");
        }
        
        //When writing response, trim off the /www root folder name from directoryPath
        response->print("\"" + directoryPath.substring(4) + entry.name() + "\"");
        (*matchCounter)++;
      }
    }

    entry.close();
  }

  // Close the directory
  directory.close();
}
