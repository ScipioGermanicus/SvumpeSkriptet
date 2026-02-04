# SvumpeSkriptet
This repo contains all code for my (mushroom) incubator, which measures temperature and humidity, sends alerts to my phone, and controls fans to optimise humidity.

This Arduino-based incubator relies on the Nano ESP32-S3R8 IoT, and uses the DHT22 AM2302as temperature- and humidity sensor. Two fans are applied for controlling humidity.

The ESP32 sends messages to my phone via a depressed Telegram Bot, who sends annoyed status reports of temperature, humidity, and run-time.

Note that v1/ files are legacy files.
