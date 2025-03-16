#ifndef HTTP_SENDER_H
#define HTTP_SENDER_H

#include <Arduino.h>
#include <AsyncHTTPRequest_Generic.h>
#include <AsyncHTTPSRequest_Generic.h>
#include <Base64.h>

// HttpSender encapsulates HTTP/HTTPS sending functionality.
// It allows setting the server URL, API token, and Basic Auth credentials via setters.
class HttpSender {
public:
    // Type definition for response callbacks.
    // Parameters: HTTP code and response string.
    typedef void (*ResponseCallback)(int httpCode, const String &response);

    HttpSender() : sendInProgress(false) {}

    // Setter for the server URL.
    void setServerUrl(const String &url) {
        serverUrl = url;
    }

    // Setter for the API token.
    void setApiToken(const String &token) {
        apiToken = token;
    }

    // Setter for Basic Authentication credentials.
    void setBasicAuth(const String &username, const String &password) {
        basicUsername = username;
        basicPassword = password;
    }

    // Sets the callback to be invoked on a successful response.
    void setSuccessCallback(ResponseCallback cb) {
        successCallback = cb;
    }

    // Sets the callback to be invoked on a failure response.
    void setFailureCallback(ResponseCallback cb) {
        failureCallback = cb;
    }

    // Returns true if a send operation is currently in progress.
    bool isSending() const {
        return sendInProgress;
    }

    // Sends the provided JSON payload via HTTP or HTTPS.
    // The protocol is determined at runtime by checking if serverUrl starts with "https".
    void sendRequest(const String &jsonPayload) {
        if (serverUrl.length() == 0) {
            Serial.println("Server URL not set!");
            return;
        }
        bool useHttps = serverUrl.startsWith("https");
        if (useHttps) {
            httpsRequest.onReadyStateChange([this](void *optParm, AsyncHTTPSRequest *request, int readyState) {
                this->handleHttpsResponse(optParm, request, readyState);
            });
            if (httpsRequest.readyState() == readyStateUnsent || httpsRequest.readyState() == readyStateDone) {
                bool openResult = httpsRequest.open("POST", serverUrl.c_str());
                if (openResult) {
                    httpsRequest.setReqHeader("Content-Type", "application/json");
                    if (apiToken.length() > 0) {
                        httpsRequest.setReqHeader("X-API-Token", apiToken.c_str());
                    }
                    if (basicUsername.length() > 0 && basicPassword.length() > 0) {
                        String credentials = basicUsername + ":" + basicPassword;
                        String base64Credentials = base64::encode(credentials);
                        httpsRequest.setReqHeader("Authorization", ("Basic " + base64Credentials).c_str());
                    }
                    httpsRequest.send(jsonPayload);
                    sendInProgress = true;
                } else {
                    Serial.println(F("Can't open HTTPS request"));
                }
            } else {
                Serial.println(F("HTTPS request not ready"));
            }
        } else {
            httpRequest.onReadyStateChange([this](void *optParm, AsyncHTTPRequest *request, int readyState) {
                this->handleHttpResponse(optParm, request, readyState);
            });
            if (httpRequest.readyState() == readyStateUnsent || httpRequest.readyState() == readyStateDone) {
                bool openResult = httpRequest.open("POST", serverUrl.c_str());
                if (openResult) {
                    httpRequest.setReqHeader("Content-Type", "application/json");
                    if (apiToken.length() > 0) {
                        httpRequest.setReqHeader("X-API-Token", apiToken.c_str());
                    }
                    if (basicUsername.length() > 0 && basicPassword.length() > 0) {
                        String credentials = basicUsername + ":" + basicPassword;
                        String base64Credentials = base64::encode(credentials);
                        httpRequest.setReqHeader("Authorization", ("Basic " + base64Credentials).c_str());
                    }
                    httpRequest.send(jsonPayload);
                    sendInProgress = true;
                } else {
                    Serial.println(F("Can't open HTTP request"));
                }
            } else {
                Serial.println(F("HTTP request not ready"));
            }
        }
    }

private:
    AsyncHTTPRequest httpRequest;
    AsyncHTTPSRequest httpsRequest;
    bool sendInProgress;

    // Configurable members (set via setters).
    String serverUrl;
    String apiToken;
    String basicUsername;
    String basicPassword;

    ResponseCallback successCallback = nullptr;
    ResponseCallback failureCallback = nullptr;

    // Internal handler for HTTP responses.
    void handleHttpResponse(void *optParm, AsyncHTTPRequest *request, int readyState) {
        if (readyState == readyStateDone) {
            int httpCode = request->responseHTTPcode();
            Serial.print("HTTP request completed. Code: ");
            Serial.println(httpCode);
            Serial.print("HTTP response: ");
            Serial.println(request->responseHTTPString());
            if (httpCode >= 200 && httpCode < 300) {
                if (successCallback) {
                    successCallback(httpCode, request->responseHTTPString());
                }
            } else {
                if (failureCallback) {
                    failureCallback(httpCode, request->responseHTTPString());
                }
            }
            sendInProgress = false;
        }
    }

    // Internal handler for HTTPS responses.
    void handleHttpsResponse(void *optParm, AsyncHTTPSRequest *request, int readyState) {
        if (readyState == readyStateDone) {
            int httpCode = request->responseHTTPcode();
            Serial.print("HTTPS request completed. Code: ");
            Serial.println(httpCode);
            Serial.print("HTTPS response: ");
            Serial.println(request->responseHTTPString());
            if (httpCode >= 200 && httpCode < 300) {
                if (successCallback) {
                    successCallback(httpCode, request->responseHTTPString());
                }
            } else {
                if (failureCallback) {
                    failureCallback(httpCode, request->responseHTTPString());
                }
            }
            sendInProgress = false;
        }
    }
};

#endif // HTTP_SENDER_H
