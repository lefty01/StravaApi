#include "StravaApi.h"

StravaApi::StravaApi(Client &client)
{
  this->client = &client;
}

StravaApi::StravaApi(Client &client, unsigned int clientId,
		     const char* secret,
		     const char* token):
  client(&client),
  clientId(clientId),
  clientSecret(secret),
  refreshToken(token),
  expiresAt(0),
  expiresIn(0)
{
}

void StravaApi::print()
{
  Serial.println("StravaApi::print()");
  Serial.println(clientId);
  Serial.println(refreshToken);
  Serial.println(bearerToken);
  Serial.println(expiresIn);
  Serial.println(expiresAt);
}

int StravaApi::isExpired()
{
  Serial.println("StravaApi::isExpired()");
  Serial.println(expiresIn);

  return 0;
}

String StravaApi::getBody()
{
  String headers;
  String body;
  bool finishedHeaders = false;
  bool currentLineIsBlank = true;
  unsigned long now = millis();
  bool avail = false;
  int ch_count = 0;

  while (millis() - now < API_TIMEOUT) {
    while (client->available()) {
      avail = finishedHeaders;
      char c = client->read();

      if (! finishedHeaders) {
	if (currentLineIsBlank && c == '\n') {
	  finishedHeaders = true;
	  //Serial.println("http header:");
	  //Serial.println(headers);
	}
	else {
	  headers = headers + c;
	}
      }
      else {
	if (ch_count < maxMessageLength)  {
	  body = body + c;
	  ch_count++;
	}
      }

      if (c == '\n') {
	currentLineIsBlank = true;
      }
      else if (c != '\r') {
	currentLineIsBlank = false;
      }
    }
  }
  return body;
}

int StravaApi::auth()
{
  String body;
  bool finishedHeaders = false;
  bool currentLineIsBlank = true;
  unsigned long now;
  bool avail;

  DynamicJsonDocument doc(256);

  doc["client_id"]     = String(this->clientId);
  doc["client_secret"] = this->clientSecret;   // 40 hex-char
  doc["refresh_token"] = this->refreshToken;   // 40 hex-char
  doc["grant_type"]    = "refresh_token";

  // Connect with twitter api over ssl
  Serial.println("trying to connect to server");
  if (client->connect(API_HOST, API_PORT)) {
    int ch_count = 0;
    Serial.println(".... connected to server");

    // Connection: close -> default on 1.0, we use 1.0 instead of 1.1 because cannot handle chunked response (yet)
    client->println(F("POST /api/v3/oauth/token HTTP/1.0"));
    client->println(F("Host: " API_HOST));
    client->println(F("User-Agent: arduino/1.0.0"));
    client->println(F("Accept: */*"));
    client->println(F("Content-Type: application/json"));
    client->print(F("Content-Length: "));
    client->println(String(measureJson(doc)));
    client->println();
    serializeJson(doc, *client);

    body = getBody();

    if (body) {

      const size_t capacity = JSON_OBJECT_SIZE(5) + 229;
      DynamicJsonDocument jsonBuffer(capacity);

      auto error = deserializeJson(jsonBuffer, body);
      //auto error = deserializeJson(jsonBuffer, *client);
      if (error) {
	Serial.println(F("Error with response: deserializeJson() failed:"));
	Serial.println(error.c_str());
	return 1;
      }

      if (jsonBuffer["access_token"] && isTokenValid(jsonBuffer["access_token"]))
	bearerToken = jsonBuffer["access_token"].as<String>();
      else {
	Serial.println(F("Error invalid bearer token"));
	return 1;
      }
      if (jsonBuffer["refresh_token"] && isTokenValid(jsonBuffer["refresh_token"]))
	refreshToken = jsonBuffer["refresh_token"].as<String>();
      else {
	Serial.println(F("Error invalid refresh token"));
	return 1;
      }

      if (jsonBuffer["expires_at"]) {
	expiresAt = jsonBuffer["expires_at"];
      }
      if (jsonBuffer["expires_in"]) {
	expiresIn = jsonBuffer["expires_in"];
      }
    }
    //} // while timeout
  }
  else {
    Serial.println("could not connect to www.strava.com");
    return 1;
  }
  return 0;
}


void StravaApi::getCommand(const String &cmd)
{
  client->println(cmd);
  client->println(F("Host: " API_HOST));
  client->println(F("User-Agent: arduino/1.0.0"));
  client->println(F("Accept: */*"));
  client->print(F("Authorization: Bearer "));
  client->println(String(bearerToken));
  client->println();
}

JsonObject StravaApi::getAthleteStats(int athleteId)
{
  String body;
  String getCmd = "GET /api/v3/athletes/" + String(athleteId) + "/stats HTTP/1.0";

  // if token expired, refresh tokene aka call auth()

  if (client->connect(API_HOST, API_PORT)) {
    Serial.println(".... connected to server");

    getCommand(getCmd);
    body = getBody();
    if (body) {
      const size_t capacity = 6*JSON_OBJECT_SIZE(5)
	+ 3*JSON_OBJECT_SIZE(6)
	+ JSON_OBJECT_SIZE(11)
	+ 2048;
      DynamicJsonDocument jsonBuffer(capacity);
      Serial.println("body:");
      Serial.println(body);
      auto error = deserializeJson(jsonBuffer, body);
      if (error) {
	Serial.println(F("Error: deserializeJson() failed:"));
	Serial.println(error.c_str());
	return {};
      }

      JsonObject ytd_run_totals = jsonBuffer["ytd_run_totals"];
      // int ytd_run_totals_count = ytd_run_totals["count"];
      // long ytd_run_totals_distance = ytd_run_totals["distance"];
      // long ytd_run_totals_moving_time = ytd_run_totals["moving_time"];
      // long ytd_run_totals_elapsed_time = ytd_run_totals["elapsed_time"];
      // int ytd_run_totals_elevation_gain = ytd_run_totals["elevation_gain"];
      //Serial.println(ytd_run_totals_distance);
      //return String(ytd_run_totals_distance/1000);
      return ytd_run_totals;
    }
  }
  return {};
}

String StravaApi::getAthlete()
{
  String body;
  String getCmd = "GET /api/v3/athlete HTTP/1.0";

  // if token expired, refresh tokene aka call auth()
  if (client->connect(API_HOST, API_PORT)) {
    Serial.println(".... connected to server");
    getCommand(getCmd);

    body = getBody();
    if (body) {
      Serial.println("body:");
      Serial.println(body);
    }
  }
  return "";
}



int StravaApi::isTokenValid(String token)
{
  // validate token format and size
  if (token.length() != 40) {
    //Serial.println(F("Error invalid token length"));
    return 0;
  }
  for (char& c : token) {
    if (!isxdigit(c)) {
      //Serial.println(F("Error invalid token format"));
      return 0;
    }
  }
  return 1;
}

