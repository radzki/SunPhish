#include <DNSServer.h>
#include <ESP8266WiFi.h>
#include <Ticker.h>
#include <vector>
#include <string>
#include <LittleFS.h>
#include <time.h>
#include <ArduinoOTA.h>

extern "C" {
    #include "user_interface.h"

    extern struct rst_info resetInfo;
    bool wifi_softap_deauth(uint8 mac[6]);
}

#include "ESPAsyncTCP.h"
#include "ESPAsyncWebServer.h"
#include "config.h"

#if!defined(ESP8266)
#error This code is designed to run on ESP8266 and ESP8266 - based boards!Please check your Tools -> Board setting.
#endif

DNSServer dnsServer;
AsyncWebServer server(80);
Ticker clientDropper;
Ticker softwareRTCTicker;
Ticker voltageMeter;
std::vector<String> voltage_reads;

signed int rtc_seconds;
signed int sleep_min_hour;
signed int sleep_max_hour;
signed int cutoff_voltage;

float last_battery_voltage;
float last_panel_voltage;
float last_temperature;

class ConnectionInfo {

    private:
        unsigned int timestamp;
    uint8 mac_address[6];

    public:
        ConnectionInfo() {
            timestamp = millis();
        }

    static void printMacAddress(uint8 * mac) {
        DEBUG_SERIAL.print(mac[0], HEX);
        DEBUG_SERIAL.print(mac[1], HEX);
        DEBUG_SERIAL.print(mac[2], HEX);
        DEBUG_SERIAL.print(mac[3], HEX);
        DEBUG_SERIAL.print(mac[4], HEX);
        DEBUG_SERIAL.print(mac[5], HEX);
    }

    uint8 * getMacAddress() {
        return mac_address;
    }

    unsigned int getTimestamp() {
        return timestamp;
    }

    void setMacAddress(uint8 * mac) {

        DEBUG_SERIAL.print("New ConnectionInfo. MAC address is = ");
        ConnectionInfo::printMacAddress(mac);
        DEBUG_SERIAL.println("");

        memcpy(mac_address, mac, 6);
    }

    static bool isMacInList(std::vector < ConnectionInfo > conn_list, uint8 * mac) {

        if (conn_list.empty()) {
            DEBUG_SERIAL.println("<isMacInList>: Empty list. Return false.");
            return false;
        }

        auto _c = conn_list.begin();

        while (_c != conn_list.end()) {

            uint8 * conn_mac = _c -> getMacAddress();

            if (memcmp(conn_mac, mac, 6) == 0) {
                return true;
            }
        }

        return false;
    }
};

// Max connections: 8
std::vector < ConnectionInfo > clients;

/* Clock utils */

int getHours() {
    return rtc_seconds / 3600 % 24;
}

int getMinutes() {
    return rtc_seconds / 60 % 60;
}

int getSeconds() {
    return rtc_seconds % 60;
}

void printTime() {
    DEBUG_SERIAL.println((String)
        "Current time: " + getHours() + "h" + getMinutes() + "m" + getSeconds() + "s");
}

void saveConfigToMemory() {
    // Saves current config on RTC memory
    system_rtc_mem_write(110, & sleep_max_hour, sizeof( & sleep_max_hour));
    system_rtc_mem_write(120, & sleep_min_hour, sizeof( & sleep_min_hour));
    system_rtc_mem_write(130, & cutoff_voltage, sizeof( & cutoff_voltage));
}

void readConfigFromMemory() {
    // Reads current config on RTC memory
    system_rtc_mem_read(110, & sleep_max_hour, sizeof( & sleep_max_hour));
    system_rtc_mem_read(120, & sleep_min_hour, sizeof( & sleep_min_hour));
    system_rtc_mem_read(130, & cutoff_voltage, sizeof( & cutoff_voltage));
}

void sleepHandler() {

    int hour = getHours();

    return;

    // TODO: SLEEP HOUR BUGADO

    // Desligar entre 22h - 8h da manha
    if (!(hour < sleep_min_hour && hour <= sleep_max_hour) ||
        (last_battery_voltage > 2.5 && (last_battery_voltage * 1000) < cutoff_voltage)) {

        DEBUG_SERIAL.println("Gotta save some energy. Let's sleep. ZZzzZZzz...");

        // Saves current timestamp on RTC memory
        system_rtc_mem_write(100, & rtc_seconds, sizeof( & rtc_seconds));

        saveConfigToMemory();

        // Maximum sleep = 30min = 1800s
        // https://forum.arduino.cc/t/esp8266-and-deep-sleep-time/677977/3
        ESP.deepSleep(SLEEPTIMEuS);
        // For testing purposes
        //ESP.deepSleep(5e6);
    }
}

void softwareRTC() {

    rtc_seconds++;
    sleepHandler();
}

/* =========== */

/* ADC Functions */
void allOutputToLow() {
    digitalWrite(TEMP_SENSOR, LOW);
    digitalWrite(VD_PANEL, LOW);
    digitalWrite(VD_BATTERY, LOW);
    digitalWrite(LED_BUILTIN, LED_OFF);
}

int readADC() {

    int value = 0;
    // 10 times sampling to prevent fluctuations
    for (int i = 0; i < 10; i++) {
        value += analogRead(ADC_IN);
        delay(5);
    }

    return (value / 10);
}

void clientDropperCallback() {

    printTime();

    if (clients.empty()) {
        return;
    }

    unsigned long currentMillis = millis();

    DEBUG_SERIAL.println("--------------");
    DEBUG_SERIAL.print("Current miliss(): ");
    DEBUG_SERIAL.println(currentMillis);
    DEBUG_SERIAL.println("Connected Clients: ");

    auto _c = clients.begin();

    while (_c != clients.end()) {

        uint8 * mac = _c -> getMacAddress();
        unsigned int timestamp = _c -> getTimestamp();

        if (currentMillis - timestamp >= 60000) {
            DEBUG_SERIAL.println("Connection expired. Sending deauth frame to client:");
            bool dauth = wifi_softap_deauth(mac);
            if (dauth) {
                DEBUG_SERIAL.println("(Deauth frame sent successfully)");
            } else {
                DEBUG_SERIAL.println("(Error sending deauth frame)");
            }

            _c = clients.erase(_c);
        } else {
            _c++;
        }

        DEBUG_SERIAL.print("MAC: ");
        ConnectionInfo::printMacAddress(mac);
        DEBUG_SERIAL.print(" - Timestamp: ");
        DEBUG_SERIAL.print(timestamp);
        DEBUG_SERIAL.println("");
    }

    DEBUG_SERIAL.println("--------------");
}

void storeMetricsToVector(){
  String line = ((String) rtc_seconds + "," + (String)last_panel_voltage + "," + (String)last_battery_voltage + 
    "," + (String)last_temperature);

  if (voltage_reads.size() >= 2000){
    std::rotate(voltage_reads.begin(), voltage_reads.begin() + 1, voltage_reads.end());
  }

  voltage_reads.push_back(line);
  
}

void voltageMeterCallback() {

    #if DEBUG
    digitalWrite(LED_BUILTIN, LED_ON);
    #endif

    float temp_offset = (20 - last_temperature) / 100;

    digitalWrite(TEMP_SENSOR, HIGH);
    delay(2500);
    last_temperature = readADC();
    DEBUG_SERIAL.print("Temperature Sensor - ADC Value: ");
    DEBUG_SERIAL.print(last_temperature);
    DEBUG_SERIAL.print(" - Celsius: ");
    last_temperature = ((last_temperature * (3.32 / 1024)) - 0.70) * 100;
    DEBUG_SERIAL.println(last_temperature);
    digitalWrite(TEMP_SENSOR, LOW);

    digitalWrite(VD_PANEL, HIGH);
    delay(2500);
    last_panel_voltage = readADC();
    DEBUG_SERIAL.print("Solar Panel Voltage - ADC Value: ");
    DEBUG_SERIAL.print(last_panel_voltage);
    DEBUG_SERIAL.print(" - Voltage: ");
    last_panel_voltage = ((last_panel_voltage * (3.32 / 1024)) / 0.39) + 0.6 + temp_offset;
    DEBUG_SERIAL.println(last_panel_voltage);
    digitalWrite(VD_PANEL, LOW);

    delay(2000);

    digitalWrite(VD_BATTERY, HIGH);
    delay(2500);
    last_battery_voltage = readADC();
    DEBUG_SERIAL.print("Battery Pack Voltage - ADC Value:  ");
    DEBUG_SERIAL.print(last_battery_voltage);
    DEBUG_SERIAL.print(" - Voltage: ");
    last_battery_voltage = ((last_battery_voltage * (3.32 / 1024)) / 0.51) + 0.6 + temp_offset;
    DEBUG_SERIAL.println(last_battery_voltage);
    digitalWrite(VD_BATTERY, LOW);

    delay(2000);

    #if DEBUG
    digitalWrite(LED_BUILTIN, LED_OFF);
    #endif

}

void resetHandler() {

    // Starts clock at noon if nothing is found on RTC Mem.
    // Also should test if deep sleep was triggered from low battery
    if (resetInfo.reason == REASON_DEEP_SLEEP_AWAKE) {

        // Back from deep sleep
        DEBUG_SERIAL.println("Wakey wakey!");

        // Reads saved seconds and increases counter with sleep time
        system_rtc_mem_read(100, & rtc_seconds, sizeof( & rtc_seconds));
        rtc_seconds += SLEEPTIME - TIMEDRIFT;

        // Saved parameters
        readConfigFromMemory();

        voltageMeterCallback();

    } else {
        rtc_seconds = RTC_START_UNIXTIME;
        sleep_min_hour = SLEEP_MIN_HOUR;
        sleep_max_hour = SLEEP_MAX_HOUR;
        cutoff_voltage = CUTOFF_VOLTAGE;
    }
}

String processor(const String &
    var) {
    if (var == "TIMESTAMP") {
        return String(rtc_seconds, DEC);
    } else if (var == "LOCALE") {
        return String(LOCALE);
    } else if (var == "BATTERY_VOLTAGE") {
        return String(last_battery_voltage);
    } else if (var == "PANEL_VOLTAGE") {
        return String(last_panel_voltage);
    } else if (var == "TEMPERATURE") {
        return String(last_temperature);
    } else if (var == "SLEEP_MAX_HOUR") {
        return String(sleep_max_hour);
    } else if (var == "SLEEP_MIN_HOUR") {
        return String(sleep_min_hour);
    } else if (var == "CUTOFF_VOLTAGE") {
        return String(cutoff_voltage);
    }
    return String();
}

class CaptiveRequestHandler: public AsyncWebHandler {
    public: CaptiveRequestHandler() {

        // Xiaomi
        server.on("/generate_204", HTTP_GET, [](AsyncWebServerRequest * request) {
            request -> send(FILESYSTEM, "/index.html");
        });
        // Windows devices
        server.on("/connecttest.txt", HTTP_GET, [](AsyncWebServerRequest * request) {
            request -> send(FILESYSTEM, "/index.html");
        });
        // Android devices?
        server.on("/gen_204", HTTP_GET, [](AsyncWebServerRequest * request) {
            request -> send(FILESYSTEM, "/index.html");
        });
        // Apple Deviecs
        server.on("/hotspot-detect.html", HTTP_GET, [](AsyncWebServerRequest * request) {
            request -> send(FILESYSTEM, "/index.html");
        });

        server.on("/config", HTTP_GET, [](AsyncWebServerRequest * request) {
            request -> send(FILESYSTEM, "/config.html", "text/html", false, processor);
        });

        server.on("/ajuda", HTTP_GET, [](AsyncWebServerRequest * request) {
            request -> send(FILESYSTEM, "/static/ajuda.pdf", "application/pdf");
        });

        server.on("/settings", HTTP_POST, [](AsyncWebServerRequest * request) {
            if (request -> hasParam("timestamp", true)) {
                String timestamp = request -> getParam("timestamp", true) -> value();
                rtc_seconds = timestamp.toInt();
            }

            if (request -> hasParam("sleep_min_hour", true)) {
                String smih = request -> getParam("sleep_min_hour", true) -> value();
                sleep_min_hour = smih.toInt();
            }

            if (request -> hasParam("sleep_max_hour", true)) {
                String smah = request -> getParam("sleep_max_hour", true) -> value();
                sleep_max_hour = smah.toInt();
            }

            if (request -> hasParam("cutoff_voltage", true)) {
                String ctv = request -> getParam("cutoff_voltage", true) -> value();
                cutoff_voltage = ctv.toInt();
            }

            request -> send(201);
        });

        server.on("/login", HTTP_POST, [](AsyncWebServerRequest * request) {
            //String login;
            //String password;

            DEBUG_SERIAL.println("Received login request");

            // Capture parameters
            // int paramsNr = request->params();
            // DEBUG_SERIAL.println(paramsNr);
            // for(int i=0;i<paramsNr;i++){
            //   AsyncWebParameter* p = request->getParam(i);
            //   DEBUG_SERIAL.print("Param name: ");
            //   DEBUG_SERIAL.println(p->name());
            //   DEBUG_SERIAL.print("Param value: ");
            //   DEBUG_SERIAL.println(p->value());
            //   DEBUG_SERIAL.println("------");
            //   DEBUG_SERIAL.println(request->hasParam("login"));
            //   DEBUG_SERIAL.println(request->hasParam("password"));
            // }

            if (request -> hasParam("login", true) && request -> hasParam("password", true)) {

                //login = request->getParam("login", true)->value();
                //password = request->getParam("password", true)->value();

                struct station_info * stat_info;
                stat_info = wifi_softap_get_station_info();

                uint8_t mac_ap[6];

                wifi_get_macaddr(SOFTAP_IF, mac_ap);

                DEBUG_SERIAL.print("MAC address is = ");
                ConnectionInfo::printMacAddress(stat_info -> bssid);
                request -> send(FILESYSTEM, "/whathappened.html");

                /* --------------------- */

                // If MAC is not on the list, push it
                if (!ConnectionInfo::isMacInList(clients, stat_info -> bssid) && clients.size() < 8) {
                    ConnectionInfo con;
                    con.setMacAddress(stat_info -> bssid);
                    clients.push_back(con);
                }
            }
        });

        // not found
        server.onNotFound([](AsyncWebServerRequest * request) {
            DEBUG_SERIAL.print("NOT FOUND HANDLER");
            request -> send(FILESYSTEM, "/index.html");
        });
    }

    virtual~CaptiveRequestHandler() {}

    bool canHandle(AsyncWebServerRequest * request) {
        return true;
    }

    void handleRequest(AsyncWebServerRequest * request) {
        String req_url = request -> url();
        DEBUG_SERIAL.print(req_url);
        request -> send(FILESYSTEM, req_url);
    }
};

void arduinoOtaStart() {

    /* OTA Flashing */
    ArduinoOTA.setHostname("esp-rdz");
    ArduinoOTA.onStart([]() {
        DEBUG_SERIAL.println("Starting OTA Flash...");
        String type;

        if (ArduinoOTA.getCommand() == U_FLASH)
            type = "sketch";
        else
            type = "filesystem";

        // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
        FILESYSTEM.end();

        DEBUG_SERIAL.println("Start updating " + type);
    });

    ArduinoOTA.onEnd([]() {
        DEBUG_SERIAL.println("OTA flashing finished!");
        delay(5000);
        ESP.restart();
    });

    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
        DEBUG_SERIAL.printf("Progress: %u%%r\n", (progress / (total / 100)));
    });

    ArduinoOTA.onError([](ota_error_t error) {
        DEBUG_SERIAL.printf("Erro [%u]: ", error);
        if (error == OTA_BEGIN_ERROR) {
            DEBUG_SERIAL.println("OTA_BEGIN_ERROR");
        } else if (error == OTA_CONNECT_ERROR) {
            DEBUG_SERIAL.println("OTA_CONNECT_ERROR");
        } else if (error == OTA_RECEIVE_ERROR) {
            DEBUG_SERIAL.println("OTA_RECEIVE_ERROR");
        } else if (error == OTA_END_ERROR) {
            DEBUG_SERIAL.println("OTA_END_ERROR");
        }
    });

    ArduinoOTA.begin();
}

void setup() {

    resetHandler();

    // Deep Sleep Reset
    pinMode(D0, WAKEUP_PULLUP);

    // Voltage Divider Pins
    pinMode(ADC_IN, INPUT);
    pinMode(TEMP_SENSOR, OUTPUT);
    pinMode(VD_PANEL, OUTPUT);
    pinMode(VD_BATTERY, OUTPUT);

    // Builtin LED
    pinMode(LED_BUILTIN, OUTPUT);

    // All OUTPUT to LOW state
    allOutputToLow();

    // Software RTC
    softwareRTCTicker.attach(1, softwareRTC);

    IPAddress apIP(8, 8, 8, 8);
    IPAddress netMsk(255, 255, 255, 0);

    #if DEBUG
    Serial.begin(9600);
    Serial.println("Started serial\n");
    #endif

    WiFi.softAPConfig(apIP, apIP, netMsk);
    WiFi.softAP("#NET-ESCURO-WIFI");
    dnsServer.start(53, "*", apIP);

    FILESYSTEM.begin();

    arduinoOtaStart();

    // Handler portal cativo
    server.addHandler(new CaptiveRequestHandler()).setFilter(ON_AP_FILTER);
    server.begin();

    // Funcao para checar se deve enviar frame deauth para usuarios conectados
    clientDropper.attach(5, clientDropperCallback);
}

void loop() {
    ArduinoOTA.handle();
    dnsServer.processNextRequest();
    // We must use some delays inside voltage meter function, so it won't work inside interrupts
    //voltageMeterCallback();
    if (rtc_seconds % 60 == 0) {
        voltageMeterCallback();
        //storeMetricsToVector();
    }
}