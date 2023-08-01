// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2022 Thomas Basler and others
 */
#include "MessageOutput.h"
#include "NtpSettings.h"
#include "Configuration.h"
#include <time.h>
#include <sntp.h>


NtpSettingsClass::NtpSettingsClass()
{
}

void NtpSettingsClass::init()
{
    struct tm now;
    getLocalTime(&now);
    MessageOutput.printf("RTC before NTP init: %s\r\n", asctime(&now));

    using std::placeholders::_1;
    sntp_set_time_sync_notification_cb(&onTimeSync);
    sntp_set_sync_interval(1800000); // every 30 minutes, default: 1h
    setServer();
    setTimezone();
}

void NtpSettingsClass::setServer()
{
    configTime(0, 0, Configuration.get().Ntp_Server);
}

void NtpSettingsClass::setTimezone()
{
    setenv("TZ", Configuration.get().Ntp_Timezone, 1);
    tzset();
}

void NtpSettingsClass::onTimeSync(timeval *tv)
{
  MessageOutput.printf("NTP time sync: %s\r\n", ctime(&tv->tv_sec));
}

NtpSettingsClass NtpSettings;