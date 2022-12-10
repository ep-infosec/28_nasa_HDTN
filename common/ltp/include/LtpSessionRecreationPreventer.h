/**
 * @file LtpSessionRecreationPreventer.h
 * @author  Brian Tomko <brian.j.tomko@nasa.gov>
 *
 * @copyright Copyright � 2021 United States Government as represented by
 * the National Aeronautics and Space Administration.
 * No copyright is claimed in the United States under Title 17, U.S.Code.
 * All Other Rights Reserved.
 *
 * @section LICENSE
 * Released under the NASA Open Source Agreement (NOSA)
 * See LICENSE.md in the source root directory for more information.
 *
 * @section DESCRIPTION
 *
 * This LtpSessionRecreationPreventer class is used to remember a desired number of
 * the most recent previously received LTP session numbers.
 * It was created during testing of sending large UDP packets with IP fragmentation to
 * help mitigate an anomaly that was seen where old closed session numbers would
 * reappear much later during a multi-session transmission.
 */

#ifndef LTP_SESSION_RECREATION_PREVENTER_H
#define LTP_SESSION_RECREATION_PREVENTER_H 1

#include <cstdint>
#include <vector>
#include <unordered_set>
#include "ltp_lib_export.h"

class LtpSessionRecreationPreventer {
private:
    LtpSessionRecreationPreventer();
public:
    
    LTP_LIB_EXPORT LtpSessionRecreationPreventer(const uint64_t numReceivedSessionsToRemember);
    LTP_LIB_EXPORT ~LtpSessionRecreationPreventer();
    
    LTP_LIB_EXPORT bool AddSession(const uint64_t newSessionNumber);
    LTP_LIB_EXPORT bool ContainsSession(const uint64_t newSessionNumber) const;

private:
    const uint64_t M_NUM_RECEIVED_SESSION_NUMBERS_TO_REMEMBER;
    std::unordered_set<uint64_t> m_previouslyReceivedSessionNumbersUnorderedSet;
    std::vector<uint64_t> m_previouslyReceivedSessionNumbersQueueVector;
    bool m_queueIsFull;
    uint64_t m_nextQueueIndex;
};

#endif // LTP_SESSION_RECREATION_PREVENTER_H

