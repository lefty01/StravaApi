#ifndef _StravaApi_h_
#define _StravaApi_h_

#include <Arduino.h>
#include <ArduinoJson.h>
#include <Client.h>
#include "_strava_com.crt.h"

#define API_HOST "www.strava.com"
#define API_PORT 443
#define API_TIMEOUT 1500

// strava_fingerprint: 72:31:7F:F5:1C:DA:32:10:28:29:C2:8C:7B:F2:5E:EA:38:0F:F0:03:E1:4C:F4:20:C2:8B:78:AE:47:84:5E:12

class StravaApi
{
  public:
    StravaApi(Client &client);
    // takes client, clientId, clientSecret, and refreshToken
    StravaApi(Client &, unsigned int, const char*, const char*);
    int auth();
    void print();
    int isExpired();
    JsonObject getAthleteStats(int);
    String getAthlete();

  private:
    Client *client;
    unsigned int clientId;
    String clientSecret;
    String refreshToken;
    String bearerToken;
    long expiresAt;
    long expiresIn;
    const int maxMessageLength = 4096;

    int isTokenValid(String);
    String getBody();
    void getCommand(const String&);
};
#endif
