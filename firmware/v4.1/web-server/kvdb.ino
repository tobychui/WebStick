/*

   Key Value database

   This is a file system based database
   that uses foldername as table name,
   filename as key and content as value

   Folder name and filename are limited to
   5 characters as SDFS requirements.
*/

//Root of the db on SD card, **must have tailing slash**
const String DB_root = "/db/";

//Clean the input for any input string
String DBCleanInput(const String& inputString) {
  String trimmedString = inputString;
  //Replae all the slash that might breaks the file system
  trimmedString.replace("/", "");
  //Trim off the space before and after the string
  trimmedString.trim();
  return trimmedString;
}

//Database init create all the required table for basic system operations
void DBInit() {
  /* Preference persistent store */
  DBNewTable("pref"); //Preference settings
  /* User Authentications Tables */
  DBNewTable("auth"); //Auth session store
  DBNewTable("user"); //User accounts store
  DBNewTable("sess"); //Session store
  /* Share System Tables */
  DBNewTable("shln");//Shared links to filename map
  DBNewTable("shfn");//Shared filename to links map
}

//Create a new Database table
void DBNewTable(String tableName) {
  tableName = DBCleanInput(tableName);
  if (!SD.exists(DB_root + tableName)) {
    SD.mkdir(DB_root + tableName);
    Serial.println("KVDB table created "+ tableName);
  }
}


//Check if a database table exists
bool DBTableExists(String tableName) {
  tableName = DBCleanInput(tableName);
  return SD.exists(DB_root + tableName);
}

//Write a key to a table, return true if succ
bool DBWrite(String tableName, String key, String value) {
  if (!DBTableExists(tableName)) {
    Serial.println("KVDB table name "+ tableName + " not exists!");
    return false;
  }
  tableName = DBCleanInput(tableName);
  key = DBCleanInput(key);
  String fsDataPath = DB_root + tableName + "/" + key;
  if (SD.exists(fsDataPath)) {
    //Entry already exists. Delete it
    SD.remove(fsDataPath);
  }

  //Write new data to it
  File targetEntry = SD.open(fsDataPath, FILE_WRITE);
  targetEntry.print(value);
  targetEntry.close();
  Serial.println("KVDB Entry written to: "+ fsDataPath);
  return true;
}

//Read from database, require table name and key
String DBRead(String tableName, String key) {
  if (!DBTableExists(tableName)) {
    return "";
  }
  tableName = DBCleanInput(tableName);
  key = DBCleanInput(key);
  String fsDataPath = DB_root + tableName + "/" + key;
  if (!SD.exists(fsDataPath)) {
    //Target not exists. Return empty string
    return "";
  }

  String value = "";
  File targetEntry = SD.open(fsDataPath, FILE_READ);
  while (targetEntry.available()) {
    value = value + targetEntry.readString();
  }

  targetEntry.close();
  return value;
}

//Check if a given key exists in the database
bool DBKeyExists(String tableName, String key) {
  if (!DBTableExists(tableName)) {
    return false;
  }
  tableName = DBCleanInput(tableName);
  key = DBCleanInput(key);
  String fsDataPath = DB_root + tableName + "/" + key;
  return SD.exists(fsDataPath);
}

//Remove the key-value item from db, return true if succ
bool DBRemove(String tableName, String key) {
  if (!DBKeyExists(tableName, key)){
    return false;
  }
  tableName = DBCleanInput(tableName);
  key = DBCleanInput(key);
  String fsDataPath = DB_root + tableName + "/" + key;
  SD.remove(fsDataPath);
  return true;
}
