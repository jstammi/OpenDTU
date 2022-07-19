// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2022 Thomas Basler and others
 */
#include "NetworkSettings.h"
#include "Configuration.h"
#include "defaults.h"
#include <WiFi.h>
#ifdef OPENDTU_ETHERNET
#include <ETH.h>
#endif

NetworkSettingsClass::NetworkSettingsClass()
    : apIp(192, 168, 4, 1)
    , apNetmask(255, 255, 255, 0)
{
    dnsServer.reset(new DNSServer());
}

void NetworkSettingsClass::init()
{
    using namespace std::placeholders;

    WiFi.onEvent(std::bind(&NetworkSettingsClass::NetworkEvent, this, _1));
    setupMode();
}

void NetworkSettingsClass::NetworkEvent(WiFiEvent_t event)
{
    switch (event) {
#ifdef OPENDTU_ETHERNET
    case ARDUINO_EVENT_ETH_START:
        Serial.println("ETH start");
        ETH.setHostname("esp32-ethernet");
        break;
    case ARDUINO_EVENT_ETH_STOP:
        Serial.println("ETH stop");
        break;
    case ARDUINO_EVENT_ETH_CONNECTED:
        Serial.println("ETH connected");
        ETH.config(INADDR_NONE, INADDR_NONE, INADDR_NONE, INADDR_NONE);
        break;
    case ARDUINO_EVENT_ETH_GOT_IP:
        Serial.println("ETH got IP");
        break;
    case ARDUINO_EVENT_ETH_DISCONNECTED:
        Serial.println("ETH disconnected");
        break;
#endif
    case ARDUINO_EVENT_WIFI_STA_CONNECTED:
        Serial.println("WiFi connected");
        break;
    case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
        Serial.println("WiFi disconnected");
        break;
    case ARDUINO_EVENT_WIFI_STA_GOT_IP:
        Serial.println("WiFi got ip");
        break;
    default:
        Serial.printf("Event: %d\n", event);
    }
}

void NetworkSettingsClass::setupMode()
{
    if (adminEnabled) {
        WiFi.mode(WIFI_AP_STA);
        String ssidString = getApName();
        WiFi.softAPConfig(apIp, apIp, apNetmask);
        WiFi.softAP((const char*)ssidString.c_str(), ACCESS_POINT_PASSWORD);
        dnsServer->setErrorReplyCode(DNSReplyCode::NoError);
        dnsServer->start(DNS_PORT, "*", WiFi.softAPIP());
        dnsServerStatus = true;
    } else {
        dnsServer->stop();
        dnsServerStatus = false;
        WiFi.mode(WIFI_STA);
    }
#ifdef OPENDTU_ETHERNET
    ETH.begin();
#endif
}

void NetworkSettingsClass::enableAdminMode()
{
    adminEnabled = true;
    adminTimeoutCounter = 0;
    setupMode();
}

String NetworkSettingsClass::getApName()
{
    uint32_t chipId = 0;
    for (int i = 0; i < 17; i += 8) {
        chipId |= ((ESP.getEfuseMac() >> (40 - i)) & 0xff) << i;
    }
    return String(ACCESS_POINT_NAME + String(chipId));
}

void NetworkSettingsClass::loop()
{
    if (millis() - lastTimerCall > 1000) {
        adminTimeoutCounter++;
        connectTimeoutTimer++;
        connectRedoTimer++;
        lastTimerCall = millis();
    }
    if (adminEnabled) {
        // Don't disable the admin mode when WiFi is not available
        if (WiFi.status() != WL_CONNECTED) {
            adminTimeoutCounter = 0;
        }
        // If WiFi is connected to AP for more than ADMIN_TIMEOUT
        // seconds, disable the internal Access Point
        if (adminTimeoutCounter > ADMIN_TIMEOUT) {
            adminEnabled = false;
            Serial.println(F("Admin mode disabled"));
            setupMode();
        }
        // It's nearly not possible to use the internal AP if the
        // WiFi is searching for an AP. So disable searching afer
        // WIFI_RECONNECT_TIMEOUT and repeat after WIFI_RECONNECT_REDO_TIMEOUT
        if (WiFi.status() == WL_CONNECTED) {
            connectTimeoutTimer = 0;
            connectRedoTimer = 0;
        } else {
            if (connectTimeoutTimer > WIFI_RECONNECT_TIMEOUT && !forceDisconnection) {
                Serial.print(F("Disable search for AP... "));
                WiFi.mode(WIFI_AP);
                Serial.println(F("done"));
                connectRedoTimer = 0;
                forceDisconnection = true;
            }
            if (connectRedoTimer > WIFI_RECONNECT_REDO_TIMEOUT && forceDisconnection) {
                Serial.print(F("Enable search for AP... "));
                WiFi.mode(WIFI_AP_STA);
                Serial.println(F("done"));
                applyConfig();
                connectTimeoutTimer = 0;
                forceDisconnection = false;
            }
        }
    }
    if (dnsServerStatus) {
        dnsServer->processNextRequest();
    }
}

void NetworkSettingsClass::applyConfig()
{
    setHostname();
    if (!strcmp(Configuration.get().WiFi_Ssid, "")) {
        return;
    }
    Serial.print(F("Configuring WiFi STA using "));
    if (strcmp(WiFi.SSID().c_str(), Configuration.get().WiFi_Ssid) || strcmp(WiFi.psk().c_str(), Configuration.get().WiFi_Password)) {
        Serial.print(F("new credentials... "));
        WiFi.begin(
            Configuration.get().WiFi_Ssid,
            Configuration.get().WiFi_Password);
    } else {
        Serial.print(F("existing credentials... "));
        WiFi.begin();
    }
    Serial.println(F("done"));
    setStaticIp();
}

void NetworkSettingsClass::setHostname()
{
    Serial.print(F("Setting Hostname... "));
    if (strcmp(Configuration.get().WiFi_Hostname, "")) {
        if (WiFi.hostname(Configuration.get().WiFi_Hostname)) {
            Serial.println(F("done"));
        } else {
            Serial.println(F("failed"));
        }
    } else {
        Serial.println(F("failed (Hostname empty)"));
    }
}

void NetworkSettingsClass::setStaticIp()
{
    if (!Configuration.get().WiFi_Dhcp) {
        Serial.print(F("Configuring WiFi STA static IP... "));
        WiFi.config(
            IPAddress(Configuration.get().WiFi_Ip),
            IPAddress(Configuration.get().WiFi_Gateway),
            IPAddress(Configuration.get().WiFi_Netmask),
            IPAddress(Configuration.get().WiFi_Dns1),
            IPAddress(Configuration.get().WiFi_Dns2));
        Serial.println(F("done"));
    }
}

NetworkSettingsClass NetworkSettings;