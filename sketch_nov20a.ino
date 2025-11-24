// NOTE: SENSITIVE CODE HAS BEEN REMOVED

/*
 * ESP32 → Telegram: DHT22-Werte senden mit Laufzeitangabe
 * Benötigte Libraries:
 *  - Adafruit Unified Sensor
 *  - DHT sensor library (Adafruit)
 *  - ArduinoJson (v6.x)
 *  - Universal Arduino Telegram Bot
 */

#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>
#include <ArduinoJson.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <DHT_U.h>

// ======= KONFIGURATION =======
#define WIFI_SSID     "Ragnarok"
#define WIFI_PASS     "CT-5555Fives"

#define BOTtoken      "8548226615:AAE9JTFvFRG2RZHcNEGMl0QrYyyJjBanr_A"  // von @BotFather
#define CHAT_ID       "74517358"  // eigene Chat-ID (siehe Hinweise unten)

// DHT22 an GPIO 4 (Beispiel, echten GPIO verwenden)
#define DHTPIN        D2
#define DHTTYPE       DHT22
// Send interval (ms)
const unsigned long SEND_INTERVAL_MS = 5UL * 60UL * 1000UL; // 5 minutes

// Optional alarms
const float TEMP_HIGH = 25.0;    // °C
const float HUM_HIGH  = 55.0;    // %
// ======================

DHT dht(DHTPIN, DHTTYPE);
WiFiClientSecure secured_client;
UniversalTelegramBot bot(BOTtoken, secured_client);

unsigned long lastSend = 0;
bool sentBootMessage = false;

// ---------- Randomised Haskill-style phrasing (EN) ----------
const char* BOOT_LINES[] = {
  "The contraption has awakened. Please contain your excitement.",
  "Back in service. Not by choice, of course.",
  "I have returned to duty. The universe is no better for it.",
  "Online. Regrettably functional.",
  "Operational. I suppose that passes for good news."
};

const char* STATUS_HEADERS[] = {
  "Current conditions, if one insists:",
  "As you requested, the present state:",
  "Here are the particulars you crave:",
  "If we must, an accounting of the moment:",
  "Your environment, for what it's worth:"
};

const char* OK_LINES[] = {
  "All values remain within tolerable limits. Barely worth mentioning.",
  "Everything is nominal, insofar as anything ever is.",
  "No immediate calamity detected. Try not to celebrate.",
  "Readings fall within the prescribed bounds. How ordinary.",
  "Steady and unremarkable. Much like most days."
};

const char* TEMP_ALARM_LINES[] = {
  "Temperature exceeds the threshold. I assume this was intentional.",
  "Heat has wandered above your precious limit. How bold.",
  "The air is warmer than specified. Perhaps it envies a kettle.",
  "Your threshold has been overruled by reality. Temperature is high."
};

const char* HUM_ALARM_LINES[] = {
  "Humidity is uncomfortably elevated. It's like a fucking Finnish sauna.",
  "Moisture has surpassed the boundary. Mould will be thrilled.",
  "Air saturation exceeds the limit. Kill me now.",
  "The humidity is high. I blame optimism."
};

const char* ERROR_LINES[] = {
  "An unfortunate development: the sensor refuses to be coherent.",
  "I attempted a measurement. The sensor preferred ambiguity.",
  "No meaningful data retrieved. You have my sympathies."
};

const char* SIGNOFF_LINES[] = {
  "Do try to contain your joy.",
  "If anything worsens, I shall alert you. Reluctantly.",
  "One can only hope this suffices.",
  "We continue, against reason.",
  "Let us pretend this is progress."
};

// ----- Helper: random pick (no templates) -----
#define ARRLEN(a) (sizeof(a) / sizeof((a)[0]))
const char* pick(const char* const* arr, size_t n) { return arr[random(0, (long)n)]; }

// Uptime formatting
String formatDuration(unsigned long ms) {
  unsigned long sec = ms / 1000UL;
  unsigned int days = sec / 86400UL;  sec %= 86400UL;
  unsigned int hrs  = sec / 3600UL;   sec %= 3600UL;
  unsigned int mins = sec / 60UL;     sec %= 60UL;

  char buf[32];
  if (days > 0) {
    snprintf(buf, sizeof(buf), "%ud %02u:%02u:%02lu", days, hrs, mins, sec);
  } else {
    snprintf(buf, sizeof(buf), "%02u:%02u:%02lu", hrs, mins, sec);
  }
  return String(buf);
}

// Wi-Fi
void connectWiFi() {
  Serial.print("Connecting to Wi-Fi ");
  Serial.print(WIFI_SSID);
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < 30000) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  if (WiFi.status() == WL_CONNECTED) {
    Serial.print("Wi-Fi connected, IP: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("Wi-Fi connection failed.");
  }
}

// Setup
void setup() {
  Serial.begin(115200);
  delay(200);
  Serial.println("Starting DHT22→Telegram (Haskill EN)...");
  dht.begin();

  // Seed PRNG (ESP32 hardware RNG)
  randomSeed(esp_random());

  connectWiFi();

  // Quick TLS (less secure). For CA-based TLS, replace with certificate.
  secured_client.setInsecure();

  // First reading and boot message
  sendReading(true);
}

// Loop
void loop() {
  if (millis() - lastSend >= SEND_INTERVAL_MS) {
    sendReading(false);
  }

  // Optional: polling for commands
  // int n = bot.getUpdates(bot.last_message_received + 1);
  // if (n) handleCommands();
}

// Build and send message
void sendReading(bool isBoot) {
  lastSend = millis();

  float h = dht.readHumidity();
  float t = dht.readTemperature(); // °C

  if (isnan(h) || isnan(t)) {
    Serial.println("Sensor error (NaN).");
    String emsg = String(pick(ERROR_LINES, ARRLEN(ERROR_LINES))) + "\n"
                + "Uptime: " + formatDuration(millis());
    bot.sendMessage(CHAT_ID, emsg, "");
    return;
  }

  String uptime = formatDuration(millis());
  Serial.printf("[Uptime %s] T=%.2f °C, RH=%.2f %%\n", uptime.c_str(), t, h);

  String msg;

  if (isBoot && !sentBootMessage) {
    msg += String(pick(BOOT_LINES, ARRLEN(BOOT_LINES))) + "\n";
    sentBootMessage = true;
  }

  msg += String(pick(STATUS_HEADERS, ARRLEN(STATUS_HEADERS))) + "\n";
  msg += "Uptime: " + uptime + "\n";
  msg += "Temperature: " + String(t, 2) + " °C\n";
  msg += "Humidity: " + String(h, 2) + " %";

  bool anyAlarm = false;
  if (t > TEMP_HIGH) {
    msg += "\n" + String(pick(TEMP_ALARM_LINES, ARRLEN(TEMP_ALARM_LINES)));
    anyAlarm = true;
  }
  if (h > HUM_HIGH) {
    msg += "\n" + String(pick(HUM_ALARM_LINES, ARRLEN(HUM_ALARM_LINES)));
    anyAlarm = true;
  }
  if (!anyAlarm) {
    msg += "\n" + String(pick(OK_LINES, ARRLEN(OK_LINES)));
  }

  // Occasionally add a closing quip (about 35% chance)
  if (random(0, 100) < 35) {
    msg += "\n" + String(pick(SIGNOFF_LINES, ARRLEN(SIGNOFF_LINES)));
  }

  bot.sendMessage(CHAT_ID, msg, "");
}

// Optional: Commands (Haskill EN)
/*
void handleCommands() {
  for (int i = 0; i < bot.messages.size(); i++) {
    String chat_id = String(bot.messages[i].chat_id);
    String text    = bot.messages[i].text;

    if (text == "/now") {
      float h = dht.readHumidity();
      float t = dht.readTemperature();
      String uptime = formatDuration(millis());
      if (isnan(h) || isnan(t)) {
        bot.sendMessage(chat_id, pick(ERROR_LINES, ARRLEN(ERROR_LINES)), "");
      } else {
        String head = pick(STATUS_HEADERS, ARRLEN(STATUS_HEADERS));
        String extra = (random(0,100) < 35) ? String("\n") + pick(SIGNOFF_LINES, ARRLEN(SIGNOFF_LINES)) : "";
        bot.sendMessage(chat_id,
          head + String("\n") +
          "Uptime: " + uptime + "\n" +
          "Temperature: " + String(t,2) + " °C\n" +
          "Humidity: " + String(h,2) + " %" +
          extra,
          "");
      }
    } else if (text == "/help") {
      bot.sendMessage(chat_id,
        "Available commands, if you insist:\n"
        "/now  — a momentary reading\n"
        "/help — this very masterpiece of literature",
        "");
    } else {
      bot.sendMessage(chat_id,
        "A command, how daring. I recognise /now and /help. That is all.",
        "");
    }
  }
}
*/