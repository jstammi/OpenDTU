// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include <time.h>

class NtpSettingsClass {
    static void onTimeSync(timeval *tv);
public:
    NtpSettingsClass();
    void init();

    void setServer();
    void setTimezone();    
};

extern NtpSettingsClass NtpSettings;