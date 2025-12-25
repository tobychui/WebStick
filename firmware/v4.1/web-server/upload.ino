/*

    Upload.ino

    This script handles file upload to the web-stick
    by default this function require authentication.
    Hence, admin.txt must be set before use

*/

void handleFileUpload(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final) {
  // make sure authenticated before allowing upload
  if (IsUserAuthed(request)) {
    String logmessage = "";
    //String logmessage = "Client:" + request->client()->remoteIP().toString() + " " + request->url();
    //Serial.println(logmessage);

    //Rewrite the filename if it is too long
    filename = trimFilename(filename);

    //Get the dir to store the file
    String dirToStore = GetPara(request, "dir");
    if (!dirToStore.startsWith("/")) {
      dirToStore = "/" + dirToStore;
    }

    if (!dirToStore.endsWith("/")) {
      dirToStore = dirToStore + "/";
    }
    dirToStore = "/www" + dirToStore;

    if (!index) {
      Serial.println("Selected Upload Dir: " + dirToStore);
      logmessage = "Upload Start: " + String(filename) + " by " + request->client()->remoteIP().toString();
      // open the file on first call and store the file handle in the request object
      if (!SD.exists(dirToStore)) {
        SD.mkdir(dirToStore);
      }

      //Already exists. Overwrite
      if (SD.exists(dirToStore + filename)) {
        SD.remove(dirToStore + filename);
      }
      request->_tempFile = SD.open(dirToStore + filename, FILE_WRITE);
      Serial.println(logmessage);
    }

    if (len) {
      // stream the incoming chunk to the opened file
      request->_tempFile.write(data, len);
      //logmessage = "Writing file: " + String(filename) + " index=" + String(index) + " len=" + String(len);
      //Serial.println(logmessage);
    }

    if (final) {
      logmessage = "Upload Complete: " + String(filename) + ",size: " + String(index + len);
      // close the file handle as the upload is now done
      request->_tempFile.close();
      Serial.println(logmessage);

      //Check if the file actually exists on SD card
      if (!SD.exists(String(dirToStore + filename))) {
        //Not found!
        SendErrorResp(request, "Write failed for " + String(filename) + ". Try a shorter name!");
        return;
      }
      request->send(200, "application/json", "ok");
    }
  } else {
    Serial.println("Auth: Failed");
    SendErrorResp(request, "unauthorized");
  }
}

/*
    Upload File Trimming

    This trim the given uploading filename to less than 32 chars
    if the filename is too long to fit on the SD card.

    The code handle UTF-8 trimming at the bytes level.
*/

//UTF-8 is varaible in length, this get how many bytes in the coming sequences
//are part of this UTF-8 word
uint8_t getUtf8CharLength(const uint8_t firstByte) {
  if ((firstByte & 0x80) == 0) {
    // Single-byte character
    return 1;
  } else if ((firstByte & 0xE0) == 0xC0) {
    // Two-byte character
    return 2;
  } else if ((firstByte & 0xF0) == 0xE0) {
    // Three-byte character
    return 3;
  } else if ((firstByte & 0xF8) == 0xF0) {
    // Four-byte character
    return 4;
  } else {
    // Invalid UTF-8 character
    return 0;
  }
}

String filterBrokenUtf8(const String& input) {
  String result;
  size_t inputLength = input.length();
  size_t i = 0;
  while (i < inputLength) {
    uint8_t firstByte = input[i];
    uint8_t charLength = getUtf8CharLength(firstByte);

    if (charLength == 0){
       //End of filter
       break;
    }
    
    // Check if the character is complete (non-broken UTF-8)
    if (i + charLength <= inputLength) {
      // Check for invalid UTF-8 continuation bytes in the character
      for (size_t j = 0; j < charLength; j++) {
        result += input[i];
        i++;
      }
    }else{
      //End of valid UTF-8 segment
      break;
    }
  }
  return result;
}

String trimFilename(String& filename) {
  //Replace all things that is not suppose to be in the filename
  filename.replace("#","");
  filename.replace("?","");
  filename.replace("&","");
  
  // Find the position of the last dot (file extension)
  int dotIndex = filename.lastIndexOf('.');

  // Check if the filename contains a dot and the extension is not at the beginning or end
  if (dotIndex > 0 && dotIndex < filename.length() - 1) {
    // Calculate the maximum length for the filename (excluding extension)
    int maxLength = 32 - (filename.length() - dotIndex - 1);

    // Truncate the filename if it's longer than the maximum length
    if (filename.length() > maxLength) {
      String trimmedFilename = filterBrokenUtf8(filename.substring(0, maxLength)) + filename.substring(dotIndex);
      return trimmedFilename;
    }
  }

  // If no truncation is needed, return the original filename
  return filename;
}
