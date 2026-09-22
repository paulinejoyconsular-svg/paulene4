#define ENABLE_USER_AUTH
#define ENABLE_DATABASE

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <FirebaseClient.h>
#include <LittleFS.h>
#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>
#include <DHT.h>
#include <time.h>

/* =====================================================
   KIMBERLY ACTIVITY 4
   DHT11 GPIO4
   FIREBASE + LITTLEFS + WIFI MANAGER
   PAULENE STYLE
   ===================================================== */

// =====================================================
// DHT11
// =====================================================

#define DHTPIN 4
#define DHTTYPE DHT11

DHT dht(DHTPIN, DHTTYPE);

// =====================================================
// FIREBASE - PAULENE PROJECT
// =====================================================

#define API_KEY "AIzaSyBe3OXujc9yOLKCfk1Gj7jxFevNegiQ-cM"

#define DATABASE_URL "https://consular-5a22d-default-rtdb.europe-west1.firebasedatabase.app"

// IMPORTANT:
// Replace these two placeholders with the Firebase
// Authentication account created in the PAULENE project.
// Do NOT post the password in chat.
#define USER_EMAIL "paulinejoyconsular@gmail.com"
#define USER_PASSWORD "christinamarie2004"

// =====================================================
// FIREBASE OBJECTS
// =====================================================

UserAuth user_auth(
    API_KEY,
    USER_EMAIL,
    USER_PASSWORD,
    3000
);

FirebaseApp app;
WiFiClientSecure ssl_client;
AsyncClientClass aClient(ssl_client);
RealtimeDatabase Database;

// =====================================================
// WEB SERVER
// =====================================================

AsyncWebServer server(80);

// =====================================================
// WIFI MANAGER
// =====================================================

const char *AP_SSID = "ESP-WIFI-MANAGER";

bool wifiManagerMode = false;

// =====================================================
// SENSOR INTERVAL
// =====================================================

const unsigned long SENSOR_INTERVAL = 10000;
unsigned long lastSensorRead = 0;

// =====================================================
// DECLARATIONS
// =====================================================

void processFirebase(AsyncResult &aResult);
bool connectToSavedWiFi();
void startWiFiManager();
void startMainWebServer();
void setupFirebase();
void sendSensorData();

String readFile(const char *path);
bool writeFile(const char *path, const String &data);
void deleteWiFiFiles();

String getDateString();
String getTimeString();

// =====================================================
// READ FILE
// =====================================================

String readFile(const char *path)
{
    if (!LittleFS.exists(path))
        return "";

    File file = LittleFS.open(path, "r");

    if (!file)
        return "";

    String data = file.readString();
    file.close();

    data.trim();

    return data;
}

// =====================================================
// WRITE FILE
// =====================================================

bool writeFile(const char *path, const String &data)
{
    File file = LittleFS.open(path, "w");

    if (!file)
    {
        Serial.print("Failed to open file: ");
        Serial.println(path);
        return false;
    }

    file.print(data);
    file.close();

    return true;
}

// =====================================================
// DELETE WIFI FILES
// =====================================================

void deleteWiFiFiles()
{
    LittleFS.remove("/ssid.txt");
    LittleFS.remove("/pass.txt");
    LittleFS.remove("/ip.txt");
    LittleFS.remove("/gateway.txt");

    Serial.println("WiFi settings deleted.");
}

// =====================================================
// CONNECT SAVED WIFI
// =====================================================

bool connectToSavedWiFi()
{
    String ssid = readFile("/ssid.txt");
    String pass = readFile("/pass.txt");
    String ip = readFile("/ip.txt");
    String gateway = readFile("/gateway.txt");

    if (ssid.length() == 0)
    {
        Serial.println("No saved WiFi credentials.");
        return false;
    }

    Serial.println();
    Serial.println("=================================");
    Serial.println("      SAVED WIFI FOUND");
    Serial.println("=================================");
    Serial.print("SSID: ");
    Serial.println(ssid);

    WiFi.mode(WIFI_STA);
    delay(300);

    if (ip.length() > 0 && gateway.length() > 0)
    {
        IPAddress local_IP;
        IPAddress gateway_IP;

        if (
            local_IP.fromString(ip) &&
            gateway_IP.fromString(gateway)
        )
        {
            IPAddress subnet(255, 255, 255, 0);

            if (
                WiFi.config(
                    local_IP,
                    gateway_IP,
                    subnet
                )
            )
            {
                Serial.println("Static IP configured.");
            }
            else
            {
                Serial.println("Static IP configuration failed.");
            }
        }
    }

    WiFi.begin(
        ssid.c_str(),
        pass.c_str()
    );

    Serial.print("Connecting to WiFi");

    unsigned long startTime = millis();

    while (
        WiFi.status() != WL_CONNECTED &&
        millis() - startTime < 20000
    )
    {
        Serial.print(".");
        delay(500);
    }

    Serial.println();

    if (WiFi.status() == WL_CONNECTED)
    {
        Serial.println();
        Serial.println("=================================");
        Serial.println("       WIFI CONNECTED");
        Serial.println("=================================");

        Serial.print("SSID: ");
        Serial.println(WiFi.SSID());

        Serial.print("IP Address: ");
        Serial.println(WiFi.localIP());

        Serial.print("Gateway: ");
        Serial.println(WiFi.gatewayIP());

        Serial.println();

        return true;
    }

    Serial.println("Failed to connect to saved WiFi.");

    WiFi.disconnect(true);
    delay(1000);

    return false;
}

// =====================================================
// WIFI MANAGER HTML
// =====================================================

const char WIFI_MANAGER_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>Kimberly WiFi Manager</title>

<style>
*{box-sizing:border-box}

body{
    margin:0;
    min-height:100vh;
    background:#fffafa;
    color:#3d3034;
    font-family:Arial,Helvetica,sans-serif;
}

.top-line{
    height:5px;
    background:#d98da8;
}

.wrapper{
    width:min(600px,90%);
    margin:0 auto;
    padding:45px 0;
}

.brand{
    font-size:12px;
    font-weight:700;
    letter-spacing:4px;
    color:#b96786;
    margin-bottom:50px;
}

.eyebrow{
    margin:0 0 10px;
    font-size:10px;
    font-weight:700;
    letter-spacing:2px;
    color:#c27b96;
}

h1{
    margin:0;
    font-family:Georgia,serif;
    font-size:48px;
    font-weight:400;
    color:#352b2f;
}

.subtitle{
    margin:15px 0 35px;
    color:#86767b;
    font-size:14px;
    line-height:1.6;
}

form{
    background:#ffffff;
    border-top:2px solid #d98da8;
    padding:28px;
}

label{
    display:block;
    margin-bottom:8px;
    font-size:10px;
    font-weight:700;
    letter-spacing:1.5px;
    color:#987983;
}

input{
    width:100%;
    padding:13px;
    margin-bottom:22px;
    border:1px solid #dfcbd2;
    background:#fff;
    outline:none;
    font-size:14px;
    color:#3d3034;
}

input:focus{
    border-color:#d98da8;
}

button{
    width:100%;
    padding:15px;
    border:none;
    background:#d98da8;
    color:white;
    font-size:11px;
    font-weight:700;
    letter-spacing:1.5px;
    cursor:pointer;
}

button:hover{
    background:#c77b98;
}

.note{
    margin-top:20px;
    font-size:11px;
    color:#9a858c;
    line-height:1.6;
}

footer{
    margin-top:45px;
    padding-top:20px;
    border-top:1px solid #eadcdf;
    font-size:9px;
    letter-spacing:2px;
    color:#ad999f;
}
</style>
</head>

<body>

<div class="top-line"></div>

<main class="wrapper">

<div class="brand">KIMBERLY</div>

<p class="eyebrow">ESP32 CONFIGURATION</p>

<h1>WiFi Manager</h1>

<p class="subtitle">
Configure the WiFi connection of the
Kimberly ESP32 DHT11 monitor.
</p>

<form method="POST" action="/">

<label for="ssid">WIFI SSID</label>
<input
    id="ssid"
    name="ssid"
    type="text"
    placeholder="Enter WiFi name"
    required
>

<label for="password">WIFI PASSWORD</label>
<input
    id="password"
    name="password"
    type="password"
    placeholder="Enter WiFi password"
    required
>

<label for="ip">STATIC IP</label>
<input
    id="ip"
    name="ip"
    type="text"
    placeholder="Leave blank for DHCP"
>

<label for="gateway">GATEWAY</label>
<input
    id="gateway"
    name="gateway"
    type="text"
    placeholder="Leave blank for DHCP"
>

<button type="submit">
SAVE & CONNECT
</button>

</form>

<p class="note">
For normal WiFi connection, leave
Static IP and Gateway blank.
The ESP32 will automatically request
an IP address from the router.
</p>

<footer>
KIMBERLY DHT11 MONITORING SYSTEM
</footer>

</main>

</body>
</html>
)rawliteral";

// =====================================================
// START WIFI MANAGER
// =====================================================

void startWiFiManager()
{
    wifiManagerMode = true;

    Serial.println();
    Serial.println("=================================");
    Serial.println("       WIFI MANAGER MODE");
    Serial.println("=================================");

    WiFi.mode(WIFI_AP);
    delay(300);

    if (!WiFi.softAP(AP_SSID))
    {
        Serial.println("ERROR: Failed to start WiFi Manager AP!");
        return;
    }

    delay(500);

    server.on(
        "/",
        HTTP_GET,
        [](AsyncWebServerRequest *request)
        {
            request->send(
                200,
                "text/html",
                WIFI_MANAGER_HTML
            );
        }
    );

    server.on(
        "/",
        HTTP_POST,
        [](AsyncWebServerRequest *request)
        {
            String ssid = "";
            String pass = "";
            String ip = "";
            String gateway = "";

            if (request->hasParam("ssid", true))
                ssid = request->getParam("ssid", true)->value();

            if (request->hasParam("password", true))
                pass = request->getParam("password", true)->value();

            if (request->hasParam("ip", true))
                ip = request->getParam("ip", true)->value();

            if (request->hasParam("gateway", true))
                gateway = request->getParam("gateway", true)->value();

            ssid.trim();
            pass.trim();
            ip.trim();
            gateway.trim();

            if (ssid.length() == 0 || pass.length() == 0)
            {
                request->send(
                    400,
                    "text/plain",
                    "SSID and password are required."
                );
                return;
            }

            writeFile("/ssid.txt", ssid);
            writeFile("/pass.txt", pass);
            writeFile("/ip.txt", ip);
            writeFile("/gateway.txt", gateway);

            request->send(
                200,
                "text/html",
                "<html><head><meta name='viewport' content='width=device-width,initial-scale=1'></head>"
                "<body style='font-family:Arial;text-align:center;padding:50px;background:#fffafa;'>"
                "<div style='background:white;padding:30px;border-top:3px solid #d98da8;max-width:500px;margin:auto;'>"
                "<h1 style='font-family:Georgia;color:#b96786;'>WiFi Saved!</h1>"
                "<p style='color:#86767b;'>The ESP32 will restart and connect to the saved WiFi.</p>"
                "<p style='color:#9a858c;'>Please wait...</p>"
                "</div></body></html>"
            );

            delay(1200);
            ESP.restart();
        }
    );

    server.begin();

    Serial.println();
    Serial.println("=================================");
    Serial.println(" WIFI MANAGER READY");
    Serial.println("=================================");

    Serial.print("Connect to WiFi: ");
    Serial.println(AP_SSID);

    Serial.print("Then open: http://");
    Serial.println(WiFi.softAPIP());

    Serial.println();
}

// =====================================================
// MAIN WEB SERVER
// =====================================================

void startMainWebServer()
{
    wifiManagerMode = false;

    server.on(
        "/",
        HTTP_GET,
        [](AsyncWebServerRequest *request)
        {
            if (LittleFS.exists("/index.html"))
            {
                request->send(
                    LittleFS,
                    "/index.html",
                    "text/html"
                );
            }
            else
            {
                request->send(
                    404,
                    "text/plain",
                    "index.html not found."
                );
            }
        }
    );

    server.on(
        "/change-wifi",
        HTTP_GET,
        [](AsyncWebServerRequest *request)
        {
            request->send(
                200,
                "text/html",
                "<html><head><meta name='viewport' content='width=device-width,initial-scale=1'></head>"
                "<body style='font-family:Arial;text-align:center;padding:50px;background:#fffafa;'>"
                "<div style='background:white;padding:30px;border-top:3px solid #d98da8;max-width:500px;margin:auto;'>"
                "<h1 style='font-family:Georgia;color:#b96786;'>Changing WiFi...</h1>"
                "<p style='color:#86767b;'>WiFi settings will be cleared.</p>"
                "<p style='color:#9a858c;'>The ESP32 will restart.</p>"
                "</div></body></html>"
            );

            delay(800);

            deleteWiFiFiles();

            ESP.restart();
        }
    );

    server.serveStatic(
        "/",
        LittleFS,
        "/"
    );

    server.begin();

    Serial.println();
    Serial.println("=================================");
    Serial.println("      MAIN WEB SERVER READY");
    Serial.println("=================================");

    Serial.print("Website: http://");
    Serial.println(WiFi.localIP());

    Serial.println();
}

// =====================================================
// FIREBASE CALLBACK
// =====================================================

void processFirebase(AsyncResult &aResult)
{
    if (!aResult.isResult())
        return;

    if (aResult.isError())
    {
        Firebase.printf(
            "Firebase Error - task: %s, msg: %s, code: %d\n",
            aResult.uid().c_str(),
            aResult.error().message().c_str(),
            aResult.error().code()
        );
    }
}

// =====================================================
// FIREBASE SETUP
// =====================================================

void setupFirebase()
{
    Serial.println();
    Serial.println("=================================");
    Serial.println("       FIREBASE SETUP");
    Serial.println("=================================");

    ssl_client.setInsecure();

    initializeApp(
        aClient,
        app,
        getAuth(user_auth),
        processFirebase,
        "authTask"
    );

    app.getApp<RealtimeDatabase>(Database);

    Database.url(DATABASE_URL);

    Serial.println("Firebase initialization started.");
    Serial.println();
}

// =====================================================
// DATE
// =====================================================

String getDateString()
{
    struct tm timeinfo;

    if (!getLocalTime(&timeinfo))
        return "1970-01-01";

    char buffer[20];

    strftime(
        buffer,
        sizeof(buffer),
        "%Y-%m-%d",
        &timeinfo
    );

    return String(buffer);
}

// =====================================================
// TIME
// =====================================================

String getTimeString()
{
    struct tm timeinfo;

    if (!getLocalTime(&timeinfo))
        return "00:00:00";

    char buffer[20];

    strftime(
        buffer,
        sizeof(buffer),
        "%H:%M:%S",
        &timeinfo
    );

    return String(buffer);
}

// =====================================================
// SEND SENSOR DATA
// =====================================================

void sendSensorData()
{
    if (!app.ready())
    {
        Serial.println("Firebase not ready yet...");
        return;
    }

    float humidity = dht.readHumidity();
    float temperature = dht.readTemperature();

    if (isnan(humidity) || isnan(temperature))
    {
        Serial.println("ERROR: Failed to read DHT11");
        return;
    }

    String date = getDateString();
    String time = getTimeString();

    String basePath =
        "/ESP32_Data/" +
        date +
        "/" +
        time;

    String temperaturePath =
        basePath + "/temperature";

    String humidityPath =
        basePath + "/humidity";

    Serial.println();
    Serial.println("=================================");
    Serial.println("       DHT11 SENSOR READING");
    Serial.println("=================================");

    Serial.print("Temperature: ");
    Serial.print(temperature, 1);
    Serial.println(" °C");

    Serial.print("Humidity: ");
    Serial.print(humidity, 1);
    Serial.println(" %");

    Serial.print("Date: ");
    Serial.println(date);

    Serial.print("Time: ");
    Serial.println(time);

    Database.set<float>(
        aClient,
        temperaturePath,
        temperature,
        processFirebase,
        "temperatureTask"
    );

    Database.set<float>(
        aClient,
        humidityPath,
        humidity,
        processFirebase,
        "humidityTask"
    );

    Serial.println("Firebase write tasks sent.");
    Serial.println("=================================");
}

// =====================================================
// SETUP
// =====================================================

void setup()
{
    Serial.begin(115200);
    delay(1000);

    Serial.println();
    Serial.println("=================================");
    Serial.println(" KIMBERLY ACTIVITY 4");
    Serial.println(" DHT11 FIREBASE MONITOR");
    Serial.println("=================================");

    // -------------------------------------------------
    // LITTLEFS
    // -------------------------------------------------

    if (!LittleFS.begin(true))
    {
        Serial.println("LittleFS mount failed!");

        while (true)
            delay(1000);
    }

    Serial.println("LittleFS ready.");

    // -------------------------------------------------
    // DHT11
    // -------------------------------------------------

    dht.begin();

    Serial.println("DHT11 initialized on GPIO4.");

    // -------------------------------------------------
    // WIFI
    // -------------------------------------------------

    bool connected =
        connectToSavedWiFi();

    if (!connected)
    {
        startWiFiManager();
        return;
    }

    // -------------------------------------------------
    // NTP
    // -------------------------------------------------

    Serial.println("Starting NTP time...");

    configTime(
        8 * 3600,
        0,
        "pool.ntp.org",
        "time.nist.gov",
        "time.google.com"
    );

    Serial.print("Waiting for time");

    struct tm timeinfo;
    int retry = 0;

    while (
        !getLocalTime(&timeinfo) &&
        retry < 20
    )
    {
        Serial.print(".");
        delay(500);
        retry++;
    }

    Serial.println();

    if (getLocalTime(&timeinfo))
    {
        Serial.println("Time synchronized.");

        Serial.print("Date: ");
        Serial.println(getDateString());

        Serial.print("Time: ");
        Serial.println(getTimeString());
    }
    else
    {
        Serial.println("WARNING: Time synchronization failed.");
    }

    // -------------------------------------------------
    // FIREBASE
    // -------------------------------------------------

    setupFirebase();

    // -------------------------------------------------
    // WEB SERVER
    // -------------------------------------------------

    startMainWebServer();

    // -------------------------------------------------
    // READY
    // -------------------------------------------------

    Serial.println();
    Serial.println("=================================");
    Serial.println("         SYSTEM READY");
    Serial.println("=================================");

    Serial.print("Website: http://");
    Serial.println(WiFi.localIP());

    Serial.println();
}

// =====================================================
// LOOP
// =====================================================

void loop()
{
    if (!wifiManagerMode)
    {
        app.loop();

        if (
            millis() - lastSensorRead >=
            SENSOR_INTERVAL
        )
        {
            lastSensorRead = millis();

            sendSensorData();
        }
    }

    delay(10);
}
