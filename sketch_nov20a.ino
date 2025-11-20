#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>
#include <ArduinoJson.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <DHT_U.h>

// --- WLAN & Telegram ---
#define WIFI_SSID     "Ragnarok"
#define WIFI_PASS     "CT-5555Fives"
#define BOTtoken      "8548226615:AAE9JTFvFRG2RZHcNEGMl0QrYyyJjBanr_A"
#define CHAT_ID       "74517358"

WiFiClientSecure client;
UniversalTelegramBot bot(BOTtoken, client);

// --- DHT22 ---
#define DHTPIN D2
#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE);

// --- Lüfter via MOSFET ---
#define FAN_PIN D12

// --- Zeitsteuerung ---
unsigned long lastStatusSend = 0;
unsigned long lastFanCycle = 0;
bool fanOn = false;

// --- Phasenstatus ---
int phase = 1;  // 1 = Kolonisation (Lüfter aus), 2 = Fruchtung (Lüfter an)

// --- 24 Stunden in Millisekunden ---
const unsigned long DAY_MS = 24UL * 60UL * 60UL * 1000UL;

// --- Intervalle ---
const unsigned long STATUS_INTERVAL = DAY_MS;           // Status alle 24h
const unsigned long FAN_ON_DURATION = 3000UL;           // 3 Sekunden an
const unsigned long FAN_CYCLE_INTERVAL = DAY_MS;        // Lüfter alle 24h

// --- Utility ---
#define COUNT_OF(x) (sizeof(x)/sizeof((x)[0]))
const char* pickRandom(const char* const* arr, size_t n) {
  if (n == 0) return "";
  long idx = random(0, (long)n);
  return arr[idx];
}

// --- WLAN verbinden ---
void connectWiFi() {
  Serial.print("Verbindung zu WiFi: ");
  Serial.println(WIFI_SSID);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\n✅ Verbunden. IP-Adresse: " + WiFi.localIP().toString());
  client.setInsecure();  // kein Zertifikat nötig
}

// --- Setup ---
void setup() {
  Serial.begin(115200);
  dht.begin();
  pinMode(FAN_PIN, OUTPUT);
  digitalWrite(FAN_PIN, LOW);

  randomSeed(millis());
  connectWiFi();

  // Erste automatische Statusmeldung nach 24h:
  lastStatusSend = millis();

  bot.sendMessage(CHAT_ID, "🍄 Bot started. Phase: 1 (Colonisation). Fans deactivated.", "");
}

// --- Loop ---
void loop() {
  // Telegram-Kommandos prüfen
  int numNewMessages = bot.getUpdates(bot.last_message_received + 1);
  if (numNewMessages > 0) {
    handleCommands(numNewMessages);
  }

  // Statusbericht alle 24 Stunden
  if (millis() - lastStatusSend > STATUS_INTERVAL) {
    sendStatus();
    lastStatusSend = millis();
  }

  // Lüftersteuerung nur in Phase 2: 3 Sekunden alle 24h
  if (phase == 2) {
    unsigned long now = millis();
    if (!fanOn && now - lastFanCycle >= FAN_CYCLE_INTERVAL) {
      digitalWrite(FAN_PIN, HIGH);
      fanOn = true;
      lastFanCycle = now;
    } 
    else if (fanOn && now - lastFanCycle >= FAN_ON_DURATION) {
      digitalWrite(FAN_PIN, LOW);
      fanOn = false;
      lastFanCycle = now;
    }
  } else {
    digitalWrite(FAN_PIN, LOW); // Sicherheit: Lüfter aus in Phase 1
  }
}

// --- Telegram-Kommandos verarbeiten ---
void handleCommands(int numNewMessages) {
  for (int i = 0; i < numNewMessages; i++) {
    String chat_id = bot.messages[i].chat_id;
    String text = bot.messages[i].text;

    if (text == "/phase1") {
      phase = 1;
      bot.sendMessage(chat_id, "🌱 Phase set to 1: colonisation.\nFans deactivated.", "");
    } 
    else if (text == "/phase2") {
      phase = 2;
      bot.sendMessage(chat_id, "🍄 Phase set to 2: fruiting.\nFans will run 3 s every 24 h.", "");
      lastFanCycle = millis();  // countdown für die nächste 24h-Periode starten
    } 
    else if (text == "/status") {
      sendStatus();
    } 
    else {
      bot.sendMessage(chat_id,
        "📋 Available commands:\n"
        "/phase1 – Set phase: colonisation (fans deactivated)\n"
        "/phase2 – Set phase: fruiting (fans 3 s / 24 h)\n"
        "/status – View status", "");
    }
  }
}

// --- Formatierte Uptime als String ---
String formatUptime(unsigned long ms) {
  unsigned long sec = ms / 1000;
  int days = sec / 86400;  sec %= 86400;
  int hours = sec / 3600;  sec %= 3600;
  int mins = sec / 60;     sec %= 60;

  char buffer[64];
  if (days > 0)
    snprintf(buffer, sizeof(buffer), "%dd %02dh %02dm %02ds", days, hours, mins, sec);
  else
    snprintf(buffer, sizeof(buffer), "%02dh %02dm %02ds", hours, mins, sec);

  return String(buffer);
}

// --- Statusbericht senden inkl. Uptime ---
void sendStatus() {
  float t = dht.readTemperature();
  float h = dht.readHumidity();

  // --- Customisable intro lines ---
  const char* introOptions[] = {
    "*I don't know you, and I don't care to know you!*\n",
    "*Stop right there criminal scum, you violated the law!*\n",
    "*Then pay with your blood!*\n",
    "*Saw a mudcrab the other day. Horrible little creatures.*\n",
    "*You have my ear citizen.*\n",
    "*Nine o'clock on the watch and all's well!*\n"
  };

  // --- Customisable closing remarks ---
  const char* outroOptions[] = {
    "\nBye!",
    "\nStop talkin'!",
    "\nStay safe, citizen.",
    "\nMay Talos guide you.",
    "\nBy the Nine Divine!"
  };

  // --- Build message ---
  String msg = pickRandom(introOptions, COUNT_OF(introOptions));
  msg += "🌡 Temperature: " + String(t, 1) + " °C\n";
  msg += "💧 Humidity: " + String(h, 1) + " %\n";
  msg += "🕒 Up-time: " + formatUptime(millis()) + "\n";
  msg += "🌱 Phase: " + String(phase) + ((phase == 1) ? " (Colonisation)" : " (Fruiting)") + "\n";
  msg += "🌀 Ventilation: " + String((phase == 2 && fanOn) ? "ON" : "OFF");
  msg += pickRandom(outroOptions, COUNT_OF(outroOptions));

  bot.sendMessage(CHAT_ID, msg, "Markdown");
}