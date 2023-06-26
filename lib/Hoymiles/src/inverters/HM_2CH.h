// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include "HM_Abstract.h"

class HM_2CH : public HM_Abstract {
public:
    explicit HM_2CH(HoymilesRadio* radio, uint64_t serial);
    static bool isValidSerial(uint64_t serial);
    String typeName();
    virtual uint8_t verifyStatisticsFragments(fragment_t fragment[], uint8_t max_fragment_id);
    const byteAssign_t* getByteAssignment();
    uint8_t getByteAssignmentSize();

private:
    uint8_t verifyStatisticsFragment(fragment_t fragment, uint8_t fragment_id, uint8_t expected_len);
};