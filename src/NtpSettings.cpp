// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2022 - 2023 Thomas Basler and others
 */
#include "MessageOutput.h"
#include "NtpSettings.h"
#include "Configuration.h"
#include <Arduino.h>
#include <time.h>
#include <sntp.h>


NtpSettingsClass::NtpSettingsClass()
{
}

void NtpSettingsClass::init()
{
    struct tm now;
    getLocalTime(&now);
    MessageOutput.printf("NTP: RTC before init: %s\r\n", asctime(&now));

    using std::placeholders::_1;
    sntp_set_time_sync_notification_cb(&onTimeSync);
    sntp_set_sync_interval(60 * 60 * 1000); // every 1h (should be default also)
    sntp_set_sync_mode(SNTP_SYNC_MODE_IMMED);
    setServer();
    setTimezone();
}

void NtpSettingsClass::setServer()
{
    configTime(0, 0, Configuration.get().Ntp.Server);
}

void NtpSettingsClass::setTimezone()
{
    setenv("TZ", Configuration.get().Ntp.Timezone, 1);
    tzset();
}

void NtpSettingsClass::onTimeSync(timeval *tv)
{
  MessageOutput.printf("NTP: time sync (%d): %s\r\n", sntp_get_sync_mode(), ctime(&tv->tv_sec));
  if (sntp_get_sync_mode() == SNTP_SYNC_MODE_IMMED) {
    sntp_set_sync_mode(SNTP_SYNC_MODE_SMOOTH);
  }
}

NtpSettingsClass NtpSettings;
