/*

    Router.ino

    This is the main router of the whole web server.
    It is like the apache.conf where you handle routing
    to different services.

    By default, all route will go to the SD card /www/ folder
*/
class MainRouter : public AsyncWebHandler {
public:
  MainRouter() {}
  virtual ~MainRouter() {}

  bool canHandle(AsyncWebServerRequest *request) const override{
    String requestURI = request->url().c_str();
    if (requestURI.equals("/upload")) {
      //File Upload Endpoint
      return false;
    } else if (requestURI.startsWith("/api/")) {
      //API paths
      return false;
    } else if (requestURI.startsWith("/share/")) {
      //Share paths
      return false;
    } 
    return true;
  }

  //Main Routing Logic Here
  void handleRequest(AsyncWebServerRequest *request) override {
    String requestURI = request->url().c_str();
      
    /* Rewrite the request path if URI contains ./ */
    if (requestURI.indexOf("./") > 0) {
      requestURI.replace("./", "");
      AsyncWebServerResponse *response = request->beginResponse(307);
      response->addHeader("Cache-Control", "no-cache");
      response->addHeader("Location", requestURI);
      request->send(response);
      return;
    }

    /* Special Routing Rules */
    //Redirect / back to index.html
    if (requestURI == "/") {
      request->redirect("/index.html");
      return;
    }

    //Special interfaces that require access controls
    if (requestURI.startsWith("/store/")) {
      //Private file storage. Not allow access
      AsyncWebServerResponse *response = request->beginResponse(401, "text/html", "403 - Forbidden");
      request->send(response);
      return;
    }

    /* Default Routing Rules */
    Serial.println("URI: " + requestURI + " | MIME: " + getMime(requestURI));
    //Check if the file exists on the SD card
    if (SD.exists("/www" + requestURI)) {
      // File exists on SD card web root
      if (IsDir("/www" + requestURI)) {
        //Requesting a directory
        if (!requestURI.endsWith("/")) {
          //Missing tailing slash
          request->redirect(requestURI + "/");
          return;
        }

        if (SD.exists("/www" + requestURI + "index.html")) {
          request->send(SDFS, "/www" + requestURI + "/index.html", "text/html", false);
        } else {
          HandleDirRender(request, requestURI, "/www" + requestURI);
        }

      } else {
        request->send(SDFS, "/www" + requestURI, getMime(requestURI), false);
      }
    } else {
      // File does not exist in web root
      AsyncResponseStream *response = request->beginResponseStream("text/html");
      Serial.println("NOT FOUND: " + requestURI);
      prettyPrintRequest(request);
      response->print("<!DOCTYPE html><html><head><title>Not Found</title></head><body>");
      response->print("<p>404 - Not Found</p>");
      response->printf("<p>Requesting http://%s with URI: %s</p>", request->host().c_str(), request->url().c_str());
      response->print("</body></html>");
      request->send(response);
    }
  }
};
