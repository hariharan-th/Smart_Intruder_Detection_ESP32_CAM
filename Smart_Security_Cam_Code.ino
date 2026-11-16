//Core Program Structure:
//cpp
// Library Inclusions
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include "esp_camera.h"
#include <UniversalTelegramBot.h>
#include <ArduinoJson.h>

// Configuration Constants
#define WIFI_SSID "YourNetworkName"
#define WIFI_PASSWORD "YourNetworkPassword"
#define BOT_TOKEN "YOUR_TELEGRAM_BOT_TOKEN"
#define CHAT_ID "YOUR_TELEGRAM_CHAT_ID"
#define PIR_PIN 13

// Global Variables
WiFiClientSecure client;
UniversalTelegramBot bot(BOT_TOKEN, client);
camera_fb_t* fb = NULL;
bool autoMode = true;
unsigned long lastMotionTime = 0;

void setup() {
    // Initialize Serial Communication
    Serial.begin(115200);
    
    // Configure PIR Sensor Pin
    pinMode(PIR_PIN, INPUT);
    
    // Initialize Camera
    camera_config_t config;
    config.ledc_channel = LEDC_CHANNEL_0;
    config.ledc_timer = LEDC_TIMER_0;
    config.pin_d0 = Y2_GPIO_NUM;
    config.pin_d1 = Y3_GPIO_NUM;
    // ... (complete pin configuration)
    config.pixel_format = PIXFORMAT_JPEG;
    config.frame_size = FRAMESIZE_UXGA; // 1600x1200
    config.jpeg_quality = 10; // 0-63, lower = higher quality
    config.fb_count = 1;
    
    esp_err_t err = esp_camera_init(&config);
    if (err != ESP_OK) {
        Serial.println("Camera init failed");
        return;
    }
    
    // Connect to WiFi
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("WiFi connected");
    
    // Configure Telegram Client
    client.setCACert(TELEGRAM_CERTIFICATE_ROOT);
    
    // Send startup message
    bot.sendMessage(CHAT_ID, "System Online", "");
}

void loop() {
    // Check for Telegram Commands
    int numNewMessages = bot.getUpdates(bot.last_message_received + 1);
    while(numNewMessages) {
        handleNewMessages(numNewMessages);
        numNewMessages = bot.getUpdates(bot.last_message_received + 1);
    }
    
    // Auto Mode: Check PIR Sensor
    if (autoMode) {
        int pirState = digitalRead(PIR_PIN);
        if (pirState == HIGH && (millis() - lastMotionTime > 10000)) {
            lastMotionTime = millis();
            captureAndSend();
        }
    }
    
    delay(100);
}

void captureAndSend() {
    // Capture Image
    fb = esp_camera_fb_get();
    if (!fb) {
        Serial.println("Camera capture failed");
        return;
    }
    
    // Send to Telegram
    String message = "Motion Detected at " + String(millis()/1000) + "s";
    bool sent = bot.sendPhoto(CHAT_ID, fb->buf, fb->len, message);
    
    // Release Frame Buffer
    esp_camera_fb_return(fb);
    
    if (sent) {
        Serial.println("Photo sent successfully");
    } else {
        Serial.println("Photo send failed");
    }
}

void handleNewMessages(int numNewMessages) {
    for (int i = 0; i < numNewMessages; i++) {
        String chat_id = String(bot.messages[i].chat_id);
        String text = bot.messages[i].text;
        
        // Verify authorized user
        if (chat_id != CHAT_ID) {
            bot.sendMessage(chat_id, "Unauthorized", "");
            continue;
        }
        
        // Process Commands
        if (text == "/start") {
            String welcome = "ESP32-CAM Surveillance Bot\n";
            welcome += "/photo - Capture image now\n";
            welcome += "/auto_on - Enable automatic mode\n";
            welcome += "/auto_off - Disable automatic mode\n";
            bot.sendMessage(chat_id, welcome, "");
        }
        
        if (text == "/photo") {
            captureAndSend();
        }
        
        if (text == "/auto_on") {
            autoMode = true;
            bot.sendMessage(chat_id, "Auto mode enabled", "");
        }
        
        if (text == "/auto_off") {
            autoMode = false;
            bot.sendMessage(chat_id, "Auto mode disabled", "");
        }
    }
}


