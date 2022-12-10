/**
 * @file Ltp.cpp
 * @author  Brian Tomko <brian.j.tomko@nasa.gov>
 *
 * @copyright Copyright © 2021 United States Government as represented by
 * the National Aeronautics and Space Administration.
 * No copyright is claimed in the United States under Title 17, U.S.Code.
 * All Other Rights Reserved.
 *
 * @section LICENSE
 * Released under the NASA Open Source Agreement (NOSA)
 * See LICENSE.md in the source root directory for more information.
 */

#include "Ltp.h"
#include <memory>
#include <boost/foreach.hpp>
#include <boost/endian/conversion.hpp>
#include <boost/lexical_cast.hpp>
#include <iostream>
#include "Sdnv.h"
#ifdef USE_CRC32C_FAST
#include <nmmintrin.h>
#endif

Ltp::session_id_t::session_id_t() : sessionOriginatorEngineId(0), sessionNumber(0) { } //a default constructor: X()
Ltp::session_id_t::session_id_t(uint64_t paramSessionOriginatorEngineId, uint64_t paramSessionNumber) :
    sessionOriginatorEngineId(paramSessionOriginatorEngineId), sessionNumber(paramSessionNumber) { }
Ltp::session_id_t::~session_id_t() noexcept { } //a destructor: ~X()
Ltp::session_id_t::session_id_t(const session_id_t& o) noexcept : sessionOriginatorEngineId(o.sessionOriginatorEngineId), sessionNumber(o.sessionNumber) { } //a copy constructor: X(const X&)
Ltp::session_id_t::session_id_t(session_id_t&& o) noexcept : sessionOriginatorEngineId(o.sessionOriginatorEngineId), sessionNumber(o.sessionNumber) { } //a move constructor: X(X&&)
Ltp::session_id_t& Ltp::session_id_t::operator=(const session_id_t& o) noexcept { //a copy assignment: operator=(const X&)
    sessionOriginatorEngineId = o.sessionOriginatorEngineId;
    sessionNumber = o.sessionNumber;
    return *this;
}
Ltp::session_id_t& Ltp::session_id_t::operator=(session_id_t && o) noexcept { //a move assignment: operator=(X&&)
    sessionOriginatorEngineId = o.sessionOriginatorEngineId;
    sessionNumber = o.sessionNumber;
    return *this;
}
Ltp::session_id_t& Ltp::session_id_t::operator=(const uint64_t o) noexcept { //assign to uint64 (for template code in LtpTimerManager)
    sessionOriginatorEngineId = o;
    sessionNumber = o;
    return *this;
}
bool Ltp::session_id_t::operator==(const session_id_t & o) const {
    return (sessionOriginatorEngineId == o.sessionOriginatorEngineId) && (sessionNumber == o.sessionNumber);
}
bool Ltp::session_id_t::operator==(const uint64_t o) const { //(for template code in LtpTimerManager)
    return (sessionOriginatorEngineId == o) && (sessionNumber == o);
}
bool Ltp::session_id_t::operator!=(const session_id_t & o) const {
    return (sessionOriginatorEngineId != o.sessionOriginatorEngineId) || (sessionNumber != o.sessionNumber);
}
bool Ltp::session_id_t::operator<(const session_id_t & o) const {
#if 1
    if (sessionOriginatorEngineId == o.sessionOriginatorEngineId) {
        return (sessionNumber < o.sessionNumber);
    }
    return (sessionOriginatorEngineId < o.sessionOriginatorEngineId);
#else
    //non-branching version of above
    const uint64_t eid = sessionOriginatorEngineId;
    const uint64_t eidRhs = o.sessionOriginatorEngineId;
    const bool engineIdEqual = (eid == eidRhs);
    const bool engineIdLtRhs = (eid < eidRhs);
    const bool sessionNumberLtRhs = (sessionNumber < o.sessionNumber);
    return (engineIdEqual * sessionNumberLtRhs) + ((!engineIdEqual) * engineIdLtRhs);
#endif
}
std::ostream& operator<<(std::ostream& os, const Ltp::session_id_t & o) {
    os << "sessionOriginatorEngineId: " << o.sessionOriginatorEngineId << ", sessionNumber: " << o.sessionNumber;
    return os;
}
uint64_t Ltp::session_id_t::Serialize(uint8_t * serialization) const {
    uint8_t * serializationBase = serialization;
    serialization += SdnvEncodeU64BufSize10(serialization, sessionOriginatorEngineId);
    serialization += SdnvEncodeU64BufSize10(serialization, sessionNumber);
    return serialization - serializationBase;
}

std::size_t Ltp::hash_session_id_t::operator()(const session_id_t& sid) const noexcept {
#ifdef USE_CRC32C_FAST
    return static_cast<std::size_t>(_mm_crc32_u64(_mm_crc32_u64(UINT32_MAX, sid.sessionNumber), sid.sessionOriginatorEngineId));
#else
    return static_cast<std::size_t>(sid.sessionNumber ^ sid.sessionOriginatorEngineId);
#endif
}


Ltp::reception_claim_t::reception_claim_t() : offset(0), length(0) { } //a default constructor: X()
Ltp::reception_claim_t::reception_claim_t(uint64_t paramOffset, uint64_t paramLength) : offset(paramOffset), length(paramLength) { }
Ltp::reception_claim_t::~reception_claim_t() { } //a destructor: ~X()
Ltp::reception_claim_t::reception_claim_t(const reception_claim_t& o) : offset(o.offset), length(o.length) { } //a copy constructor: X(const X&)
Ltp::reception_claim_t::reception_claim_t(reception_claim_t&& o) : offset(o.offset), length(o.length) { } //a move constructor: X(X&&)
Ltp::reception_claim_t& Ltp::reception_claim_t::operator=(const reception_claim_t& o) { //a copy assignment: operator=(const X&)
    offset = o.offset;
    length = o.length;
    return *this;
}
Ltp::reception_claim_t& Ltp::reception_claim_t::operator=(reception_claim_t && o) { //a move assignment: operator=(X&&)
    offset = o.offset;
    length = o.length;
    return *this;
}
bool Ltp::reception_claim_t::operator==(const reception_claim_t & o) const {
    return (offset == o.offset) && (length == o.length);
}
bool Ltp::reception_claim_t::operator!=(const reception_claim_t & o) const {
    return (offset != o.offset) || (length != o.length);
}
std::ostream& operator<<(std::ostream& os, const Ltp::reception_claim_t & o) {
    os << "offset: " << o.offset << ", length: " << o.length;
    return os;
}
uint64_t Ltp::reception_claim_t::Serialize(uint8_t * serialization) const {
    uint8_t * serializationBase = serialization;
    serialization += SdnvEncodeU64BufSize10(serialization, offset);
    serialization += SdnvEncodeU64BufSize10(serialization, length);
    return serialization - serializationBase;
}

Ltp::report_segment_t::report_segment_t() : reportSerialNumber(0), checkpointSerialNumber(0), upperBound(0), lowerBound(0) { } //a default constructor: X()
Ltp::report_segment_t::report_segment_t(uint64_t paramReportSerialNumber, uint64_t paramCheckpointSerialNumber, uint64_t paramUpperBound, uint64_t paramLowerBound, const std::vector<reception_claim_t> & paramReceptionClaims) :
    reportSerialNumber(paramReportSerialNumber), 
    checkpointSerialNumber(paramCheckpointSerialNumber),
    upperBound(paramUpperBound),
    lowerBound(paramLowerBound),
    receptionClaims(paramReceptionClaims) {}
Ltp::report_segment_t::report_segment_t(uint64_t paramReportSerialNumber, uint64_t paramCheckpointSerialNumber, uint64_t paramUpperBound, uint64_t paramLowerBound, std::vector<reception_claim_t> && paramReceptionClaims) :
    reportSerialNumber(paramReportSerialNumber),
    checkpointSerialNumber(paramCheckpointSerialNumber),
    upperBound(paramUpperBound),
    lowerBound(paramLowerBound),
    receptionClaims(std::move(paramReceptionClaims)) {}
Ltp::report_segment_t::~report_segment_t() { } //a destructor: ~X()
Ltp::report_segment_t::report_segment_t(const report_segment_t& o) : 
    reportSerialNumber(o.reportSerialNumber), checkpointSerialNumber(o.checkpointSerialNumber),
    upperBound(o.upperBound), lowerBound(o.lowerBound), receptionClaims(o.receptionClaims) { } //a copy constructor: X(const X&)
Ltp::report_segment_t::report_segment_t(report_segment_t&& o) : 
    reportSerialNumber(o.reportSerialNumber), checkpointSerialNumber(o.checkpointSerialNumber),
    upperBound(o.upperBound), lowerBound(o.lowerBound), receptionClaims(std::move(o.receptionClaims)) { } //a move constructor: X(X&&)
Ltp::report_segment_t& Ltp::report_segment_t::operator=(const report_segment_t& o) { //a copy assignment: operator=(const X&)
    reportSerialNumber = o.reportSerialNumber;
    checkpointSerialNumber = o.checkpointSerialNumber;
    upperBound = o.upperBound;
    lowerBound = o.lowerBound;
    receptionClaims = o.receptionClaims;
    return *this;
}
Ltp::report_segment_t& Ltp::report_segment_t::operator=(report_segment_t && o) { //a move assignment: operator=(X&&)
    reportSerialNumber = o.reportSerialNumber;
    checkpointSerialNumber = o.checkpointSerialNumber;
    upperBound = o.upperBound;
    lowerBound = o.lowerBound;
    receptionClaims = std::move(o.receptionClaims);
    return *this;
}
bool Ltp::report_segment_t::operator==(const report_segment_t & o) const {
    return (reportSerialNumber == o.reportSerialNumber) && (checkpointSerialNumber == o.checkpointSerialNumber)
        && (upperBound == o.upperBound) && (lowerBound == o.lowerBound)
        && (receptionClaims == o.receptionClaims);
}
bool Ltp::report_segment_t::operator!=(const report_segment_t & o) const {
    return (reportSerialNumber != o.reportSerialNumber) || (checkpointSerialNumber != o.checkpointSerialNumber)
        || (upperBound != o.upperBound) || (lowerBound != o.lowerBound)
        || (receptionClaims != o.receptionClaims);
}
std::ostream& operator<<(std::ostream& os, const Ltp::report_segment_t & o) {
    os << "report serial number: " << o.reportSerialNumber << ", checkpoint serial number: " << o.checkpointSerialNumber << std::endl;
    os << "upper bound: " << o.upperBound << ", lower bound: " << o.lowerBound << std::endl;
    os << "claims:" << std::endl;
    for (std::vector<Ltp::reception_claim_t>::const_iterator it = o.receptionClaims.cbegin(); it != o.receptionClaims.cend(); ++it) {
        os << *it << std::endl;
    }
    return os;
}
uint64_t Ltp::report_segment_t::Serialize(uint8_t * serialization) const {
    uint8_t * serializationBase = serialization;
    serialization += SdnvEncodeU64BufSize10(serialization, reportSerialNumber);
    serialization += SdnvEncodeU64BufSize10(serialization, checkpointSerialNumber);
    serialization += SdnvEncodeU64BufSize10(serialization, upperBound);
    serialization += SdnvEncodeU64BufSize10(serialization, lowerBound);
    serialization += SdnvEncodeU64BufSize10(serialization, receptionClaims.size());
    
    for (std::vector<reception_claim_t>::const_iterator it = receptionClaims.cbegin(); it != receptionClaims.cend(); ++it) {
        serialization += it->Serialize(serialization);
    }
    return serialization - serializationBase;
}
uint64_t Ltp::report_segment_t::GetMaximumDataRequiredForSerialization() const {
    return (5 * 10) + (receptionClaims.size() * (2 * 10)); //5 sdnvs * 10 bytes sdnv max + reception claims * 2sdnvs per claim
}

Ltp::ltp_extension_t::ltp_extension_t() : tag(0) { } //a default constructor: X()
Ltp::ltp_extension_t::~ltp_extension_t() { } //a destructor: ~X()
Ltp::ltp_extension_t::ltp_extension_t(const ltp_extension_t& o) : tag(o.tag), valueVec(o.valueVec) { } //a copy constructor: X(const X&)
Ltp::ltp_extension_t::ltp_extension_t(ltp_extension_t&& o) : tag(o.tag), valueVec(std::move(o.valueVec)) { } //a move constructor: X(X&&)
Ltp::ltp_extension_t& Ltp::ltp_extension_t::operator=(const ltp_extension_t& o) { //a copy assignment: operator=(const X&)
    tag = o.tag;
    valueVec = o.valueVec;
    return *this;
}
Ltp::ltp_extension_t& Ltp::ltp_extension_t::operator=(ltp_extension_t && o) { //a move assignment: operator=(X&&)
    tag = o.tag;
    valueVec = std::move(o.valueVec);
    return *this;
}
bool Ltp::ltp_extension_t::operator==(const ltp_extension_t & o) const {
    return (tag == o.tag) && (valueVec == o.valueVec);
}
bool Ltp::ltp_extension_t::operator!=(const ltp_extension_t & o) const {
    return (tag != o.tag) || (valueVec != o.valueVec);
}
void Ltp::ltp_extension_t::AppendSerialize(std::vector<uint8_t> & serialization) const {
    serialization.push_back(tag);

    //sdnv encode length (valueVec.size())
    const uint64_t originalSize = serialization.size();
    serialization.resize(originalSize + 10); //10 is largest sdnv buffer required for encode
    const unsigned int outputSizeBytesLengthSdnv = SdnvEncodeU64BufSize10(&serialization[originalSize], valueVec.size());
    serialization.resize(originalSize + outputSizeBytesLengthSdnv);

    //data copy valueVec
    serialization.insert(serialization.end(), valueVec.begin(), valueVec.end()); //concatenate
}
uint64_t Ltp::ltp_extension_t::Serialize(uint8_t * serialization) const {
    *serialization++ = tag;

    //sdnv encode length (valueVec.size())
    const uint64_t sdnvSizeBytes = SdnvEncodeU64BufSize10(serialization, valueVec.size());
    serialization += sdnvSizeBytes;

    //data copy valueVec
    memcpy(serialization, valueVec.data(), valueVec.size()); //concatenate

    return 1 + sdnvSizeBytes + valueVec.size();
}

Ltp::ltp_extensions_t::ltp_extensions_t() { } //a default constructor: X()
Ltp::ltp_extensions_t::~ltp_extensions_t() { } //a destructor: ~X()
Ltp::ltp_extensions_t::ltp_extensions_t(const ltp_extensions_t& o) : extensionsVec(o.extensionsVec) { } //a copy constructor: X(const X&)
Ltp::ltp_extensions_t::ltp_extensions_t(ltp_extensions_t&& o) : extensionsVec(std::move(o.extensionsVec)) { } //a move constructor: X(X&&)
Ltp::ltp_extensions_t& Ltp::ltp_extensions_t::operator=(const ltp_extensions_t& o) { //a copy assignment: operator=(const X&)
    extensionsVec = o.extensionsVec;
    return *this;
}
Ltp::ltp_extensions_t& Ltp::ltp_extensions_t::operator=(ltp_extensions_t && o) { //a move assignment: operator=(X&&)
    extensionsVec = std::move(o.extensionsVec);
    return *this;
}
bool Ltp::ltp_extensions_t::operator==(const ltp_extensions_t & o) const {
    return (extensionsVec == o.extensionsVec);
}
bool Ltp::ltp_extensions_t::operator!=(const ltp_extensions_t & o) const {
    return (extensionsVec != o.extensionsVec);
}
void Ltp::ltp_extensions_t::AppendSerialize(std::vector<uint8_t> & serialization) const {
    for (std::vector<ltp_extension_t>::const_iterator it = extensionsVec.cbegin(); it != extensionsVec.cend(); ++it) {
        it->AppendSerialize(serialization);
    }
}
uint64_t Ltp::ltp_extensions_t::Serialize(uint8_t * serialization) const {
    uint8_t * serializationBase = serialization;
    for (std::vector<ltp_extension_t>::const_iterator it = extensionsVec.cbegin(); it != extensionsVec.cend(); ++it) {
        serialization += it->Serialize(serialization);
    }
    return serialization - serializationBase;
}
uint64_t Ltp::ltp_extensions_t::GetMaximumDataRequiredForSerialization() const {
    uint64_t maximumBytesRequired = extensionsVec.size() * 11; //tag plus 10 bytes sdnv max
    for (std::vector<ltp_extension_t>::const_iterator it = extensionsVec.cbegin(); it != extensionsVec.cend(); ++it) {
        maximumBytesRequired += it->valueVec.size();
    }
    return maximumBytesRequired;
}

Ltp::data_segment_metadata_t::data_segment_metadata_t() : clientServiceId(0), offset(0), length(0), checkpointSerialNumber(NULL), reportSerialNumber(NULL) {}
Ltp::data_segment_metadata_t::data_segment_metadata_t(uint64_t paramClientServiceId, uint64_t paramOffset, uint64_t paramLength, uint64_t * paramCheckpointSerialNumber, uint64_t * paramReportSerialNumber) :
    clientServiceId(paramClientServiceId),
    offset(paramOffset),
    length(paramLength),
    checkpointSerialNumber(paramCheckpointSerialNumber),
    reportSerialNumber(paramReportSerialNumber) {}
bool Ltp::data_segment_metadata_t::operator==(const data_segment_metadata_t & o) const {
    if (checkpointSerialNumber) {
        if (o.checkpointSerialNumber) {
            if (*checkpointSerialNumber != *o.checkpointSerialNumber) {
                return false;
            }
        }
        else { //o.checkpointSerialNumber == NULL
            return false;
        }
    }
    else { //checkpointSerialNumber == NULL
        if (o.checkpointSerialNumber != NULL) {
            return false;
        }
    }
    return (clientServiceId == o.clientServiceId) && (offset == o.offset) && (length == o.length);
}
bool Ltp::data_segment_metadata_t::operator!=(const data_segment_metadata_t & o) const {
    return !(*this == o);
}
uint64_t Ltp::data_segment_metadata_t::Serialize(uint8_t * serialization) const {
    uint8_t * serializationBase = serialization;
    serialization += SdnvEncodeU64BufSize10(serialization, clientServiceId);
    serialization += SdnvEncodeU64BufSize10(serialization, offset);
    serialization += SdnvEncodeU64BufSize10(serialization, length);
    if (checkpointSerialNumber && reportSerialNumber) {
        serialization += SdnvEncodeU64BufSize10(serialization, *checkpointSerialNumber);
        serialization += SdnvEncodeU64BufSize10(serialization, *reportSerialNumber);
    }
    return serialization - serializationBase;
}
uint64_t Ltp::data_segment_metadata_t::GetMaximumDataRequiredForSerialization() const {
    return (3 * 10) + ((static_cast<bool>(checkpointSerialNumber && reportSerialNumber)) * (static_cast<uint8_t>(2 * 10))); //5 sdnvs * 10 bytes sdnv max + reception claims * 2sdnvs per claim
}

Ltp::Ltp() {
    InitRx();
}
Ltp::~Ltp() {

}

void Ltp::SetDataSegmentContentsReadCallback(const DataSegmentContentsReadCallback_t & callback) {
    m_dataSegmentContentsReadCallback = callback;
}
void Ltp::SetReportSegmentContentsReadCallback(const ReportSegmentContentsReadCallback_t & callback) {
    m_reportSegmentContentsReadCallback = callback;
}
void Ltp::SetReportAcknowledgementSegmentContentsReadCallback(const ReportAcknowledgementSegmentContentsReadCallback_t & callback) {
    m_reportAcknowledgementSegmentContentsReadCallback = callback;
}
void Ltp::SetCancelSegmentContentsReadCallback(const CancelSegmentContentsReadCallback_t & callback) {
    m_cancelSegmentContentsReadCallback = callback;
}
void Ltp::SetCancelAcknowledgementSegmentContentsReadCallback(const CancelAcknowledgementSegmentContentsReadCallback_t & callback) {
    m_cancelAcknowledgementSegmentContentsReadCallback = callback;
}

void Ltp::InitRx() {
    m_mainRxState = LTP_MAIN_RX_STATE::READ_HEADER;
    m_headerRxState = LTP_HEADER_RX_STATE::READ_CONTROL_BYTE;
    m_trailerRxState = LTP_TRAILER_RX_STATE::READ_ONE_TRAILER_EXTENSION_TAG_BYTE;
    m_dataSegmentRxState = LTP_DATA_SEGMENT_RX_STATE::READ_CLIENT_SERVICE_ID_SDNV;
    m_reportSegmentRxState = LTP_REPORT_SEGMENT_RX_STATE::READ_REPORT_SERIAL_NUMBER_SDNV;
    //m_reportAcknowledgementSegmentRxState = LTP_REPORT_ACKNOWLEDGEMENT_SEGMENT_RX_STATE::READ_REPORT_SERIAL_NUMBER_SDNV;
    m_sdnvTempVec.reserve(32); //critical for hardware accelerated decode (min size 16 (sizeof(__m128i)) to prevent out of bounds
    m_sdnvTempVec.clear();
}

bool Ltp::IsAtBeginningState() const { //unit testing convenience function
    return ((m_mainRxState == LTP_MAIN_RX_STATE::READ_HEADER) && (m_headerRxState == LTP_HEADER_RX_STATE::READ_CONTROL_BYTE));
}

void Ltp::SetBeginningState() {
    m_mainRxState = LTP_MAIN_RX_STATE::READ_HEADER;
    m_headerRxState = LTP_HEADER_RX_STATE::READ_CONTROL_BYTE;
}


void Ltp::HandleReceivedChar(const uint8_t rxVal, std::string & errorMessage) {
    HandleReceivedChars(&rxVal, 1, errorMessage);
}


bool Ltp::HandleReceivedChars(const uint8_t * rxVals, std::size_t numChars, std::string & errorMessage, SessionOriginatorEngineIdDecodedCallback_t * sessionOriginatorEngineIdDecodedCallbackPtr) {
    while (numChars) {
        --numChars;
        const uint8_t rxVal = *rxVals++;
        const LTP_MAIN_RX_STATE mainRxState = m_mainRxState; //const for optimization
        if (mainRxState == LTP_MAIN_RX_STATE::READ_HEADER) {
            const LTP_HEADER_RX_STATE headerRxState = m_headerRxState; //const for optimization
            if (headerRxState == LTP_HEADER_RX_STATE::READ_CONTROL_BYTE) {
                const uint8_t ltpVersion = rxVal >> 4;
                if (ltpVersion != 0) {
                    errorMessage = "error ltp version not 0.. got " + boost::lexical_cast<std::string>((int)ltpVersion);
                    return false;
                }
                else {
                    m_segmentTypeFlags = rxVal & 0x0f;
                    m_sdnvTempVec.clear();

#if 1 //this is the sdnv 2-array-element decode (batch operation) version
                    uint64_t * const decodedSdnvs = &m_sessionId.sessionOriginatorEngineId; //start of the "2 element array"
                    uint64_t numBytesTakenToDecodeThisSdnvArray;
                    bool decodeErrorDetected;
                    const unsigned int numValuesActuallyDecoded = SdnvDecodeArrayU64(rxVals, numBytesTakenToDecodeThisSdnvArray, decodedSdnvs, 2, numChars, decodeErrorDetected);
                    if (decodeErrorDetected) {
                        errorMessage = "error in shortcut LTP_HEADER_RX_STATE::READ_SESSION_ORIGINATOR_ENGINE_ID_SDNV, error decoding sessionOriginatorEngineId or sessionNumber";
                        return false;
                    }
                    if ((sessionOriginatorEngineIdDecodedCallbackPtr) && (numValuesActuallyDecoded)) { //if numValuesActuallyDecoded is 1 or 2 then sessionOriginatorEngineId was decoded
                        (*sessionOriginatorEngineIdDecodedCallbackPtr)(m_sessionId.sessionOriginatorEngineId);
                    }

                    //it turns out that the LTP_HEADER_RX_STATE will be identical to READ_SESSION_ORIGINATOR_ENGINE_ID_SDNV + numValuesActuallyDecoded so no LUT is needed
                    m_headerRxState = static_cast<LTP_HEADER_RX_STATE>(static_cast<unsigned int>(LTP_HEADER_RX_STATE::READ_SESSION_ORIGINATOR_ENGINE_ID_SDNV) + numValuesActuallyDecoded); //set next state

                    numChars -= numBytesTakenToDecodeThisSdnvArray;
                    rxVals += numBytesTakenToDecodeThisSdnvArray;
#else //below is the working non-batch read version
                    //shortcut READ_SESSION_ORIGINATOR_ENGINE_ID_SDNV (and then maybe READ_SESSION_NUMBER_SDNV)
                    if (numChars >= 16) { //shortcut/optimization to avoid reading populating m_sdnvTempVec, just decode from rxVals if there's enough bytes remaining 
                        uint8_t sdnvSize;
                        m_sessionId.sessionOriginatorEngineId = SdnvDecodeU64(rxVals, &sdnvSize, numChars);
                        if (sdnvSize == 0) {
                            errorMessage = "error in shortcut LTP_HEADER_RX_STATE::READ_SESSION_ORIGINATOR_ENGINE_ID_SDNV, sdnvSize is 0";
                            return false;
                        }
                        else { //success READ_SESSION_ORIGINATOR_ENGINE_ID_SDNV 
                            numChars -= sdnvSize;
                            rxVals += sdnvSize;
                            if (sessionOriginatorEngineIdDecodedCallbackPtr) {
                                (*sessionOriginatorEngineIdDecodedCallbackPtr)(m_sessionId.sessionOriginatorEngineId);
                            }
                            //shortcut READ_SESSION_NUMBER_SDNV
                            if (numChars >= 16) { //shortcut/optimization to avoid reading populating m_sdnvTempVec, just decode from rxVals if there's enough bytes remaining 
                                m_sessionId.sessionNumber = SdnvDecodeU64(rxVals, &sdnvSize, numChars);
                                if (sdnvSize == 0) {
                                    errorMessage = "error in shortcut LTP_HEADER_RX_STATE::READ_SESSION_NUMBER_SDNV, sdnvSize is 0";
                                    return false;
                                }
                                else { //success READ_SESSION_NUMBER_SDNV
                                    numChars -= sdnvSize;
                                    rxVals += sdnvSize;
                                    m_headerRxState = LTP_HEADER_RX_STATE::READ_NUM_EXTENSIONS_BYTE;
                                }
                            }
                            else { //not enough bytes, populate m_sdnvTempVec and then decode sdnv
                                m_headerRxState = LTP_HEADER_RX_STATE::READ_SESSION_NUMBER_SDNV;
                            }
                        }
                    }
                    else { //not enough bytes, populate m_sdnvTempVec and then decode sdnv
                        m_headerRxState = LTP_HEADER_RX_STATE::READ_SESSION_ORIGINATOR_ENGINE_ID_SDNV;
                    }
#endif
                }
            }
            else if (headerRxState == LTP_HEADER_RX_STATE::READ_SESSION_ORIGINATOR_ENGINE_ID_SDNV) {
                m_sdnvTempVec.push_back(rxVal);
                if (m_sdnvTempVec.size() > 10) {
                    errorMessage = "error in LTP_HEADER_RX_STATE::READ_SESSION_ORIGINATOR_ENGINE_ID_SDNV, sdnv > 10 bytes";
                    return false;
                }
                else if ((rxVal & 0x80) == 0) { //if msbit is a 0 then stop
                    uint8_t sdnvSize;
                    m_sessionId.sessionOriginatorEngineId = SdnvDecodeU64(m_sdnvTempVec.data(), &sdnvSize, m_sdnvTempVec.capacity());
                    if (sdnvSize != m_sdnvTempVec.size()) {
                        errorMessage = "error in LTP_HEADER_RX_STATE::READ_SESSION_ORIGINATOR_ENGINE_ID_SDNV, sdnvSize != m_sdnvTempVec.size()";
                        return false;
                    }
                    else {
                        if (sessionOriginatorEngineIdDecodedCallbackPtr) {
                            (*sessionOriginatorEngineIdDecodedCallbackPtr)(m_sessionId.sessionOriginatorEngineId);
                        }
                        m_sdnvTempVec.clear();
                        m_headerRxState = LTP_HEADER_RX_STATE::READ_SESSION_NUMBER_SDNV;
                    }
                }
            }
            else if (headerRxState == LTP_HEADER_RX_STATE::READ_SESSION_NUMBER_SDNV) {
                m_sdnvTempVec.push_back(rxVal);
                if (m_sdnvTempVec.size() > 10) {
                    errorMessage = "error in LTP_HEADER_RX_STATE::READ_SESSION_NUMBER_SDNV, sdnv > 10 bytes";
                    return false;
                }
                else if ((rxVal & 0x80) == 0) { //if msbit is a 0 then stop
                    uint8_t sdnvSize;
                    m_sessionId.sessionNumber = SdnvDecodeU64(m_sdnvTempVec.data(), &sdnvSize, m_sdnvTempVec.capacity());
                    if (sdnvSize != m_sdnvTempVec.size()) {
                        errorMessage = "error in LTP_HEADER_RX_STATE::READ_SESSION_NUMBER_SDNV, sdnvSize != m_sdnvTempVec.size()";
                        return false;
                    }
                    else {
                        m_headerRxState = LTP_HEADER_RX_STATE::READ_NUM_EXTENSIONS_BYTE;
                    }
                }
            }
            else if (headerRxState == LTP_HEADER_RX_STATE::READ_NUM_EXTENSIONS_BYTE) { //msb
                m_numHeaderExtensionTlvs = rxVal >> 4;
                m_numTrailerExtensionTlvs = rxVal & 0x0f;
                m_headerExtensions.extensionsVec.clear();
                m_headerExtensions.extensionsVec.reserve(m_numHeaderExtensionTlvs);
                m_trailerExtensions.extensionsVec.clear();
                m_trailerExtensions.extensionsVec.reserve(m_numTrailerExtensionTlvs);
                if (m_numHeaderExtensionTlvs) {
                    m_headerRxState = LTP_HEADER_RX_STATE::READ_ONE_HEADER_EXTENSION_TAG_BYTE;
                }
                else {
                    if (const uint8_t * newRxVals = NextStateAfterHeaderExtensions(rxVals, numChars, errorMessage)) {
                        rxVals = newRxVals;
                    }
                    else {
                        return false;
                    }
                }
            }
            else if (headerRxState == LTP_HEADER_RX_STATE::READ_ONE_HEADER_EXTENSION_TAG_BYTE) {
#if (__cplusplus >= 201703L)
                ltp_extension_t & ext = m_headerExtensions.extensionsVec.emplace_back();
                ext.tag = rxVal;
#else
                m_headerExtensions.extensionsVec.emplace_back();
                m_headerExtensions.extensionsVec.back().tag = rxVal;
#endif
                m_sdnvTempVec.clear();
                m_headerRxState = LTP_HEADER_RX_STATE::READ_ONE_HEADER_EXTENSION_LENGTH_SDNV;
            }
            else if (headerRxState == LTP_HEADER_RX_STATE::READ_ONE_HEADER_EXTENSION_LENGTH_SDNV) {
                m_sdnvTempVec.push_back(rxVal);
                if (m_sdnvTempVec.size() > 10) {
                    errorMessage = "error in LTP_HEADER_RX_STATE::READ_ONE_HEADER_EXTENSION_LENGTH_SDNV, sdnv > 10 bytes";
                    return false;
                }
                else if ((rxVal & 0x80) == 0) { //if msbit is a 0 then stop
                    uint8_t sdnvSize;
                    m_currentHeaderExtensionLength = SdnvDecodeU64(m_sdnvTempVec.data(), &sdnvSize, m_sdnvTempVec.capacity());
                    if (sdnvSize != m_sdnvTempVec.size()) {
                        errorMessage = "error in LTP_HEADER_RX_STATE::READ_ONE_HEADER_EXTENSION_LENGTH_SDNV, sdnvSize != m_sdnvTempVec.size()";
                        return false;
                    }
                    else if (m_currentHeaderExtensionLength == 0) {
                        if (m_headerExtensions.extensionsVec.size() == m_numHeaderExtensionTlvs) {
                            if (const uint8_t * newRxVals = NextStateAfterHeaderExtensions(rxVals, numChars, errorMessage)) {
                                rxVals = newRxVals;
                            }
                            else {
                                return false;
                            }
                        }
                        else {
                            m_headerRxState = LTP_HEADER_RX_STATE::READ_ONE_HEADER_EXTENSION_TAG_BYTE;
                        }
                    }
                    else {
                        m_headerExtensions.extensionsVec.back().valueVec.reserve(m_currentHeaderExtensionLength);
                        m_headerRxState = LTP_HEADER_RX_STATE::READ_ONE_HEADER_EXTENSION_VALUE;
                    }
                }
            }
            else if (headerRxState == LTP_HEADER_RX_STATE::READ_ONE_HEADER_EXTENSION_VALUE) {
                std::vector<uint8_t> & valueVec = m_headerExtensions.extensionsVec.back().valueVec;
                valueVec.push_back(rxVal);
                if (valueVec.size() == m_currentHeaderExtensionLength) {
                    if (m_headerExtensions.extensionsVec.size() == m_numHeaderExtensionTlvs) {
                        if (const uint8_t * newRxVals = NextStateAfterHeaderExtensions(rxVals, numChars, errorMessage)) {
                            rxVals = newRxVals;
                        }
                        else {
                            return false;
                        }
                    }
                    else {
                        m_headerRxState = LTP_HEADER_RX_STATE::READ_ONE_HEADER_EXTENSION_TAG_BYTE;
                    }
                }
            }
        }
        else if (mainRxState == LTP_MAIN_RX_STATE::READ_DATA_SEGMENT_CONTENT) {
            const LTP_DATA_SEGMENT_RX_STATE dataSegmentRxState = m_dataSegmentRxState; //const for optimization
            if (dataSegmentRxState == LTP_DATA_SEGMENT_RX_STATE::READ_CLIENT_SERVICE_ID_SDNV) {
                m_sdnvTempVec.push_back(rxVal);
                if (m_sdnvTempVec.size() > 10) {
                    errorMessage = "error in LTP_DATA_SEGMENT_RX_STATE::READ_CLIENT_SERVICE_ID_SDNV, sdnv > 10 bytes";
                    return false;
                }
                else if ((rxVal & 0x80) == 0) { //if msbit is a 0 then stop
                    uint8_t sdnvSize;
                    m_dataSegmentMetadata.clientServiceId = SdnvDecodeU64(m_sdnvTempVec.data(), &sdnvSize, m_sdnvTempVec.capacity());
                    if (sdnvSize != m_sdnvTempVec.size()) {
                        errorMessage = "error in LTP_DATA_SEGMENT_RX_STATE::READ_CLIENT_SERVICE_ID_SDNV, sdnvSize != m_sdnvTempVec.size()";
                        return false;
                    }
                    else {
                        m_sdnvTempVec.clear();
                        m_dataSegmentRxState = LTP_DATA_SEGMENT_RX_STATE::READ_OFFSET_SDNV;
                    }
                }
            }
            else if (dataSegmentRxState == LTP_DATA_SEGMENT_RX_STATE::READ_OFFSET_SDNV) {
                m_sdnvTempVec.push_back(rxVal);
                if (m_sdnvTempVec.size() > 10) {
                    errorMessage = "error in LTP_DATA_SEGMENT_RX_STATE::READ_OFFSET_SDNV, sdnv > 10 bytes";
                    return false;
                }
                else if ((rxVal & 0x80) == 0) { //if msbit is a 0 then stop
                    uint8_t sdnvSize;
                    m_dataSegmentMetadata.offset = SdnvDecodeU64(m_sdnvTempVec.data(), &sdnvSize, m_sdnvTempVec.capacity());
                    if (sdnvSize != m_sdnvTempVec.size()) {
                        errorMessage = "error in LTP_DATA_SEGMENT_RX_STATE::READ_OFFSET_SDNV, sdnvSize != m_sdnvTempVec.size()";
                        return false;
                    }
                    else {
                        m_sdnvTempVec.clear();
                        m_dataSegmentRxState = LTP_DATA_SEGMENT_RX_STATE::READ_LENGTH_SDNV;
                    }
                }
            }
            else if (dataSegmentRxState == LTP_DATA_SEGMENT_RX_STATE::READ_LENGTH_SDNV) {
                m_sdnvTempVec.push_back(rxVal);
                if (m_sdnvTempVec.size() > 10) {
                    errorMessage = "error in LTP_DATA_SEGMENT_RX_STATE::READ_LENGTH_SDNV, sdnv > 10 bytes";
                    return false;
                }
                else if ((rxVal & 0x80) == 0) { //if msbit is a 0 then stop
                    uint8_t sdnvSize;
                    m_dataSegmentMetadata.length = SdnvDecodeU64(m_sdnvTempVec.data(), &sdnvSize, m_sdnvTempVec.capacity());
                    if (sdnvSize != m_sdnvTempVec.size()) {
                        errorMessage = "error in LTP_DATA_SEGMENT_RX_STATE::READ_LENGTH_SDNV, sdnvSize != m_sdnvTempVec.size()";
                        return false;
                    }
                    else if (m_dataSegmentMetadata.length == 0) { //not sure if this is correct
                        errorMessage = "error in LTP_DATA_SEGMENT_RX_STATE::READ_LENGTH_SDNV, length == 0";
                        return false;
                    }
                    else if ((m_segmentTypeFlags >= 1) && (m_segmentTypeFlags <= 3)) { //checkpoint
                        m_sdnvTempVec.clear();
                        m_dataSegmentRxState = LTP_DATA_SEGMENT_RX_STATE::READ_CHECKPOINT_SERIAL_NUMBER_SDNV;
                        m_dataSegmentMetadata.checkpointSerialNumber = &m_dataSegmentMetadata.tmpCheckpointSerialNumber;
                        m_dataSegmentMetadata.reportSerialNumber = &m_dataSegmentMetadata.tmpReportSerialNumber;
                    }
                    else {
                        m_dataSegment_clientServiceData.clear();
                        m_dataSegment_clientServiceData.reserve(m_dataSegmentMetadata.length); //todo make sure cant crash
                        m_dataSegmentRxState = LTP_DATA_SEGMENT_RX_STATE::READ_CLIENT_SERVICE_DATA;
                        m_dataSegmentMetadata.checkpointSerialNumber = NULL;
                        m_dataSegmentMetadata.reportSerialNumber = NULL;
                    }
                }
            }
            else if (dataSegmentRxState == LTP_DATA_SEGMENT_RX_STATE::READ_CHECKPOINT_SERIAL_NUMBER_SDNV) {
                m_sdnvTempVec.push_back(rxVal);
                if (m_sdnvTempVec.size() > 10) {
                    errorMessage = "error in LTP_DATA_SEGMENT_RX_STATE::READ_CHECKPOINT_SERIAL_NUMBER_SDNV, sdnv > 10 bytes";
                    return false;
                }
                else if ((rxVal & 0x80) == 0) { //if msbit is a 0 then stop
                    uint8_t sdnvSize;
                    m_dataSegmentMetadata.tmpCheckpointSerialNumber = SdnvDecodeU64(m_sdnvTempVec.data(), &sdnvSize, m_sdnvTempVec.capacity());
                    if (sdnvSize != m_sdnvTempVec.size()) {
                        errorMessage = "error in LTP_DATA_SEGMENT_RX_STATE::READ_CHECKPOINT_SERIAL_NUMBER_SDNV, sdnvSize != m_sdnvTempVec.size()";
                        return false;
                    }
                    else {
                        m_sdnvTempVec.clear();
                        m_dataSegmentRxState = LTP_DATA_SEGMENT_RX_STATE::READ_REPORT_SERIAL_NUMBER_SDNV;
                    }
                }
            }
            else if (dataSegmentRxState == LTP_DATA_SEGMENT_RX_STATE::READ_REPORT_SERIAL_NUMBER_SDNV) {
                m_sdnvTempVec.push_back(rxVal);
                if (m_sdnvTempVec.size() > 10) {
                    errorMessage = "error in LTP_DATA_SEGMENT_RX_STATE::READ_REPORT_SERIAL_NUMBER_SDNV, sdnv > 10 bytes";
                    return false;
                }
                else if ((rxVal & 0x80) == 0) { //if msbit is a 0 then stop
                    uint8_t sdnvSize;
                    m_dataSegmentMetadata.tmpReportSerialNumber = SdnvDecodeU64(m_sdnvTempVec.data(), &sdnvSize, m_sdnvTempVec.capacity());
                    if (sdnvSize != m_sdnvTempVec.size()) {
                        errorMessage = "error in LTP_DATA_SEGMENT_RX_STATE::READ_REPORT_SERIAL_NUMBER_SDNV, sdnvSize != m_sdnvTempVec.size()";
                        return false;
                    }
                    else {
                        m_dataSegment_clientServiceData.clear();
                        m_dataSegment_clientServiceData.reserve(m_dataSegmentMetadata.length); //todo make sure cant crash
                        m_dataSegmentRxState = LTP_DATA_SEGMENT_RX_STATE::READ_CLIENT_SERVICE_DATA;
                    }
                }
            }
            else if (dataSegmentRxState == LTP_DATA_SEGMENT_RX_STATE::READ_CLIENT_SERVICE_DATA) {
                m_dataSegment_clientServiceData.push_back(rxVal);
                if (m_dataSegment_clientServiceData.size() == m_dataSegmentMetadata.length) {


                    if (m_numTrailerExtensionTlvs) { //todo should callback be after trailer
                        m_trailerRxState = LTP_TRAILER_RX_STATE::READ_ONE_TRAILER_EXTENSION_TAG_BYTE;
                        m_mainRxState = LTP_MAIN_RX_STATE::READ_TRAILER;
                    }
                    else {
                        //callback data segment
                        if (m_dataSegmentContentsReadCallback) {
                            m_dataSegmentContentsReadCallback(m_segmentTypeFlags, m_sessionId, m_dataSegment_clientServiceData, m_dataSegmentMetadata, m_headerExtensions, m_trailerExtensions);
                        }
                        SetBeginningState();
                    }
                }
                else {
                    const std::size_t bytesRemainingToCopy = m_dataSegmentMetadata.length - m_dataSegment_clientServiceData.size(); //guaranteed to be at least 1 from "if" above
                    const std::size_t bytesToCopy = std::min(numChars, bytesRemainingToCopy - 1); //leave last byte to go through the state machine
                    if (bytesToCopy) {
                        m_dataSegment_clientServiceData.insert(m_dataSegment_clientServiceData.end(), rxVals, rxVals + bytesToCopy); //concatenate
                        rxVals += bytesToCopy;
                        numChars -= bytesToCopy;
                    }
                }
            }
        }
        else if (mainRxState == LTP_MAIN_RX_STATE::READ_REPORT_SEGMENT_CONTENT) {
            const LTP_REPORT_SEGMENT_RX_STATE reportSegmentRxState = m_reportSegmentRxState; //const for optimization
            if (reportSegmentRxState == LTP_REPORT_SEGMENT_RX_STATE::READ_REPORT_SERIAL_NUMBER_SDNV) {
                m_sdnvTempVec.push_back(rxVal);
                if (m_sdnvTempVec.size() > 10) {
                    errorMessage = "error in LTP_REPORT_SEGMENT_RX_STATE::READ_REPORT_SERIAL_NUMBER_SDNV, sdnv > 10 bytes";
                    return false;
                }
                else if ((rxVal & 0x80) == 0) { //if msbit is a 0 then stop
                    uint8_t sdnvSize;
                    m_reportSegment.reportSerialNumber = SdnvDecodeU64(m_sdnvTempVec.data(), &sdnvSize, m_sdnvTempVec.capacity());
                    if (sdnvSize != m_sdnvTempVec.size()) {
                        errorMessage = "error in LTP_REPORT_SEGMENT_RX_STATE::READ_REPORT_SERIAL_NUMBER_SDNV, sdnvSize != m_sdnvTempVec.size()";
                        return false;
                    }
                    else {
                        m_sdnvTempVec.clear();
                        m_reportSegmentRxState = LTP_REPORT_SEGMENT_RX_STATE::READ_CHECKPOINT_SERIAL_NUMBER_SDNV;
                    }
                }
            }
            else if (reportSegmentRxState == LTP_REPORT_SEGMENT_RX_STATE::READ_CHECKPOINT_SERIAL_NUMBER_SDNV) {
                m_sdnvTempVec.push_back(rxVal);
                if (m_sdnvTempVec.size() > 10) {
                    errorMessage = "error in LTP_REPORT_SEGMENT_RX_STATE::READ_CHECKPOINT_SERIAL_NUMBER_SDNV, sdnv > 10 bytes";
                    return false;
                }
                else if ((rxVal & 0x80) == 0) { //if msbit is a 0 then stop
                    uint8_t sdnvSize;
                    m_reportSegment.checkpointSerialNumber = SdnvDecodeU64(m_sdnvTempVec.data(), &sdnvSize, m_sdnvTempVec.capacity());
                    if (sdnvSize != m_sdnvTempVec.size()) {
                        errorMessage = "error in LTP_REPORT_SEGMENT_RX_STATE::READ_CHECKPOINT_SERIAL_NUMBER_SDNV, sdnvSize != m_sdnvTempVec.size()";
                        return false;
                    }
                    else {
                        m_sdnvTempVec.clear();
                        m_reportSegmentRxState = LTP_REPORT_SEGMENT_RX_STATE::READ_UPPER_BOUND_SDNV;
                    }
                }
            }
            else if (reportSegmentRxState == LTP_REPORT_SEGMENT_RX_STATE::READ_UPPER_BOUND_SDNV) {
                m_sdnvTempVec.push_back(rxVal);
                if (m_sdnvTempVec.size() > 10) {
                    errorMessage = "error in LTP_REPORT_SEGMENT_RX_STATE::READ_UPPER_BOUND_SDNV, sdnv > 10 bytes";
                    return false;
                }
                else if ((rxVal & 0x80) == 0) { //if msbit is a 0 then stop
                    uint8_t sdnvSize;
                    m_reportSegment.upperBound = SdnvDecodeU64(m_sdnvTempVec.data(), &sdnvSize, m_sdnvTempVec.capacity());
                    if (sdnvSize != m_sdnvTempVec.size()) {
                        errorMessage = "error in LTP_REPORT_SEGMENT_RX_STATE::READ_UPPER_BOUND_SDNV, sdnvSize != m_sdnvTempVec.size()";
                        return false;
                    }
                    else {
                        m_sdnvTempVec.clear();
                        m_reportSegmentRxState = LTP_REPORT_SEGMENT_RX_STATE::READ_LOWER_BOUND_SDNV;
                    }
                }
            }
            else if (reportSegmentRxState == LTP_REPORT_SEGMENT_RX_STATE::READ_LOWER_BOUND_SDNV) {
                m_sdnvTempVec.push_back(rxVal);
                if (m_sdnvTempVec.size() > 10) {
                    errorMessage = "error in LTP_REPORT_SEGMENT_RX_STATE::READ_LOWER_BOUND_SDNV, sdnv > 10 bytes";
                    return false;
                }
                else if ((rxVal & 0x80) == 0) { //if msbit is a 0 then stop
                    uint8_t sdnvSize;
                    m_reportSegment.lowerBound = SdnvDecodeU64(m_sdnvTempVec.data(), &sdnvSize, m_sdnvTempVec.capacity());
                    if (sdnvSize != m_sdnvTempVec.size()) {
                        errorMessage = "error in LTP_REPORT_SEGMENT_RX_STATE::READ_LOWER_BOUND_SDNV, sdnvSize != m_sdnvTempVec.size()";
                        return false;
                    }
                    else {
                        m_sdnvTempVec.clear();
                        m_reportSegmentRxState = LTP_REPORT_SEGMENT_RX_STATE::READ_RECEPTION_CLAIM_COUNT_SDNV;
                    }
                }
            }
            else if (reportSegmentRxState == LTP_REPORT_SEGMENT_RX_STATE::READ_RECEPTION_CLAIM_COUNT_SDNV) {
                m_sdnvTempVec.push_back(rxVal);
                if (m_sdnvTempVec.size() > 10) {
                    errorMessage = "error in LTP_REPORT_SEGMENT_RX_STATE::READ_RECEPTION_CLAIM_COUNT_SDNV, sdnv > 10 bytes";
                    return false;
                }
                else if ((rxVal & 0x80) == 0) { //if msbit is a 0 then stop
                    uint8_t sdnvSize;
                    m_reportSegment.tmpReceptionClaimCount = SdnvDecodeU64(m_sdnvTempVec.data(), &sdnvSize, m_sdnvTempVec.capacity());
                    if (sdnvSize != m_sdnvTempVec.size()) {
                        errorMessage = "error in LTP_REPORT_SEGMENT_RX_STATE::READ_RECEPTION_CLAIM_COUNT_SDNV, sdnvSize != m_sdnvTempVec.size()";
                        return false;
                    }
                    else if (m_reportSegment.tmpReceptionClaimCount == 0) { //must be 1 or more (The content of an RS comprises one or more data reception claims)
                        errorMessage = "error in LTP_REPORT_SEGMENT_RX_STATE::READ_RECEPTION_CLAIM_COUNT_SDNV, count == 0";
                        return false;
                    }
                    else {
                        m_sdnvTempVec.clear();
                        m_reportSegment.receptionClaims.clear();
                        m_reportSegment.receptionClaims.reserve(m_reportSegment.tmpReceptionClaimCount);
                        m_reportSegmentRxState = LTP_REPORT_SEGMENT_RX_STATE::READ_ONE_RECEPTION_CLAIM_OFFSET_SDNV;
                    }
                }
            }
            else if (reportSegmentRxState == LTP_REPORT_SEGMENT_RX_STATE::READ_ONE_RECEPTION_CLAIM_OFFSET_SDNV) {
                m_sdnvTempVec.push_back(rxVal);
                if (m_sdnvTempVec.size() > 10) {
                    errorMessage = "error in LTP_REPORT_SEGMENT_RX_STATE::READ_ONE_RECEPTION_CLAIM_OFFSET_SDNV, sdnv > 10 bytes";
                    return false;
                }
                else if ((rxVal & 0x80) == 0) { //if msbit is a 0 then stop
                    uint8_t sdnvSize;
                    const uint64_t claimOffset = SdnvDecodeU64(m_sdnvTempVec.data(), &sdnvSize, m_sdnvTempVec.capacity());
                    if (sdnvSize != m_sdnvTempVec.size()) {
                        errorMessage = "error in LTP_REPORT_SEGMENT_RX_STATE::READ_ONE_RECEPTION_CLAIM_OFFSET_SDNV, sdnvSize != m_sdnvTempVec.size()";
                        return false;
                    }
                    else {
                        m_reportSegment.receptionClaims.resize(m_reportSegment.receptionClaims.size() + 1);
                        m_reportSegment.receptionClaims.back().offset = claimOffset;
                        m_sdnvTempVec.clear();
                        m_reportSegmentRxState = LTP_REPORT_SEGMENT_RX_STATE::READ_ONE_RECEPTION_CLAIM_LENGTH_SDNV;
                    }
                }
            }
            else if (reportSegmentRxState == LTP_REPORT_SEGMENT_RX_STATE::READ_ONE_RECEPTION_CLAIM_LENGTH_SDNV) {
                m_sdnvTempVec.push_back(rxVal);
                if (m_sdnvTempVec.size() > 10) {
                    errorMessage = "error in LTP_REPORT_SEGMENT_RX_STATE::READ_ONE_RECEPTION_CLAIM_LENGTH_SDNV, sdnv > 10 bytes";
                    return false;
                }
                else if ((rxVal & 0x80) == 0) { //if msbit is a 0 then stop
                    uint8_t sdnvSize;
                    const uint64_t claimLength = SdnvDecodeU64(m_sdnvTempVec.data(), &sdnvSize, m_sdnvTempVec.capacity());
                    if (sdnvSize != m_sdnvTempVec.size()) {
                        errorMessage = "error in LTP_REPORT_SEGMENT_RX_STATE::READ_ONE_RECEPTION_CLAIM_LENGTH_SDNV, sdnvSize != m_sdnvTempVec.size()";
                        return false;
                    }
                    else if (claimLength == 0) { //must be 1 or more (A reception claim's length shall never be less than 1)
                        errorMessage = "error in LTP_REPORT_SEGMENT_RX_STATE::READ_ONE_RECEPTION_CLAIM_LENGTH_SDNV, count == 0";
                        return false;
                    }
                    else {
                        m_reportSegment.receptionClaims.back().length = claimLength;
                        m_sdnvTempVec.clear();
                        if (m_reportSegment.receptionClaims.size() < m_reportSegment.tmpReceptionClaimCount) {
                            m_reportSegmentRxState = LTP_REPORT_SEGMENT_RX_STATE::READ_ONE_RECEPTION_CLAIM_OFFSET_SDNV;
                        }
                        else if (m_numTrailerExtensionTlvs) {
                            m_trailerRxState = LTP_TRAILER_RX_STATE::READ_ONE_TRAILER_EXTENSION_TAG_BYTE;
                            m_mainRxState = LTP_MAIN_RX_STATE::READ_TRAILER;
                        }
                        else {
                            //callback report segment
                            if (m_reportSegmentContentsReadCallback) {
                                m_reportSegmentContentsReadCallback(m_sessionId, m_reportSegment, m_headerExtensions, m_trailerExtensions);
                            }
                            SetBeginningState();
                        }
                    }
                }
            }
        }
        else if (mainRxState == LTP_MAIN_RX_STATE::READ_REPORT_ACKNOWLEDGEMENT_SEGMENT_CONTENT) {
            m_sdnvTempVec.push_back(rxVal);
            if (m_sdnvTempVec.size() > 10) {
                errorMessage = "error in LTP_MAIN_RX_STATE::READ_REPORT_ACKNOWLEDGEMENT_SEGMENT_CONTENT, sdnv > 10 bytes";
                return false;
            }
            else if ((rxVal & 0x80) == 0) { //if msbit is a 0 then stop
                uint8_t sdnvSize;
                m_reportAcknowledgementSegment_reportSerialNumber = SdnvDecodeU64(m_sdnvTempVec.data(), &sdnvSize, m_sdnvTempVec.capacity());
                if (sdnvSize != m_sdnvTempVec.size()) {
                    errorMessage = "error in LTP_MAIN_RX_STATE::READ_REPORT_ACKNOWLEDGEMENT_SEGMENT_CONTENT, sdnvSize != m_sdnvTempVec.size()";
                    return false;
                }
                else if (m_numTrailerExtensionTlvs) {
                    m_trailerRxState = LTP_TRAILER_RX_STATE::READ_ONE_TRAILER_EXTENSION_TAG_BYTE;
                    m_mainRxState = LTP_MAIN_RX_STATE::READ_TRAILER;
                }
                else {
                    //callback report acknowledgement segment
                    if (m_reportAcknowledgementSegmentContentsReadCallback) {
                        m_reportAcknowledgementSegmentContentsReadCallback(m_sessionId, m_reportAcknowledgementSegment_reportSerialNumber, m_headerExtensions, m_trailerExtensions);
                    }
                    SetBeginningState();
                }
            }

        }
        else if (mainRxState == LTP_MAIN_RX_STATE::READ_CANCEL_SEGMENT_CONTENT_BYTE) {
            m_cancelSegment_reasonCode = rxVal;
            if (m_numTrailerExtensionTlvs) {
                m_trailerRxState = LTP_TRAILER_RX_STATE::READ_ONE_TRAILER_EXTENSION_TAG_BYTE;
                m_mainRxState = LTP_MAIN_RX_STATE::READ_TRAILER;
            }
            else {
                //callback cancel segment
                if (m_cancelSegmentContentsReadCallback) {
                    m_cancelSegmentContentsReadCallback(m_sessionId, (static_cast<CANCEL_SEGMENT_REASON_CODES>(m_cancelSegment_reasonCode)), (m_segmentTypeFlags == (static_cast<uint8_t>(LTP_SEGMENT_TYPE_FLAGS::CANCEL_SEGMENT_FROM_BLOCK_SENDER))), m_headerExtensions, m_trailerExtensions);
                }
                SetBeginningState();
            }
        }
        else if (mainRxState == LTP_MAIN_RX_STATE::READ_TRAILER) {
            const LTP_TRAILER_RX_STATE trailerRxState = m_trailerRxState;
            if (trailerRxState == LTP_TRAILER_RX_STATE::READ_ONE_TRAILER_EXTENSION_TAG_BYTE) {
#if (__cplusplus >= 201703L)
                ltp_extension_t & ext = m_trailerExtensions.extensionsVec.emplace_back();
                ext.tag = rxVal;
#else
                m_trailerExtensions.extensionsVec.emplace_back();
                m_trailerExtensions.extensionsVec.back().tag = rxVal;
#endif
                m_sdnvTempVec.clear();
                m_trailerRxState = LTP_TRAILER_RX_STATE::READ_ONE_TRAILER_EXTENSION_LENGTH_SDNV;
            }
            else if (trailerRxState == LTP_TRAILER_RX_STATE::READ_ONE_TRAILER_EXTENSION_LENGTH_SDNV) {
                m_sdnvTempVec.push_back(rxVal);
                if (m_sdnvTempVec.size() > 10) {
                    errorMessage = "error in LTP_TRAILER_RX_STATE::READ_ONE_TRAILER_EXTENSION_LENGTH_SDNV, sdnv > 10 bytes";
                    return false;
                }
                else if ((rxVal & 0x80) == 0) { //if msbit is a 0 then stop
                    uint8_t sdnvSize;
                    m_currentTrailerExtensionLength = SdnvDecodeU64(m_sdnvTempVec.data(), &sdnvSize, m_sdnvTempVec.capacity());
                    if (sdnvSize != m_sdnvTempVec.size()) {
                        errorMessage = "error in LTP_TRAILER_RX_STATE::READ_ONE_TRAILER_EXTENSION_LENGTH_SDNV, sdnvSize != m_sdnvTempVec.size()";
                        return false;
                    }
                    else if (m_currentTrailerExtensionLength == 0) {
                        if (m_trailerExtensions.extensionsVec.size() == m_numTrailerExtensionTlvs) {
                            if(!NextStateAfterTrailerExtensions(errorMessage)) return false;
                        }
                        else {
                            m_trailerRxState = LTP_TRAILER_RX_STATE::READ_ONE_TRAILER_EXTENSION_TAG_BYTE;
                        }
                    }
                    else {
                        m_trailerExtensions.extensionsVec.back().valueVec.reserve(m_currentTrailerExtensionLength);
                        m_trailerRxState = LTP_TRAILER_RX_STATE::READ_ONE_TRAILER_EXTENSION_VALUE;
                    }
                }
            }
            else if (trailerRxState == LTP_TRAILER_RX_STATE::READ_ONE_TRAILER_EXTENSION_VALUE) {
                std::vector<uint8_t> & valueVec = m_trailerExtensions.extensionsVec.back().valueVec;
                valueVec.push_back(rxVal);
                if (valueVec.size() == m_currentTrailerExtensionLength) {
                    if (m_trailerExtensions.extensionsVec.size() == m_numTrailerExtensionTlvs) {
                        if (!NextStateAfterTrailerExtensions(errorMessage)) return false;
                    }
                    else {
                        m_trailerRxState = LTP_TRAILER_RX_STATE::READ_ONE_TRAILER_EXTENSION_TAG_BYTE; //continue loop
                    }
                }
            }
        }
    }
    return true;
}

const uint8_t * Ltp::NextStateAfterHeaderExtensions(const uint8_t * rxVals, std::size_t & numChars, std::string & errorMessage) {
    if ((m_segmentTypeFlags & 0xd) == 0xd) { //CAx (cancel ack) with no contents
        if (m_numTrailerExtensionTlvs) {
            m_trailerRxState = LTP_TRAILER_RX_STATE::READ_ONE_TRAILER_EXTENSION_TAG_BYTE;
            m_mainRxState = LTP_MAIN_RX_STATE::READ_TRAILER;
        }
        else {
            //callback cancel acknowledgement segment
            if (m_cancelAcknowledgementSegmentContentsReadCallback) {
                m_cancelAcknowledgementSegmentContentsReadCallback(m_sessionId, (m_segmentTypeFlags == (static_cast<uint8_t>(LTP_SEGMENT_TYPE_FLAGS::CANCEL_ACK_SEGMENT_TO_BLOCK_SENDER))), m_headerExtensions, m_trailerExtensions);
            }
            SetBeginningState();
        }
    }
    else if ((m_segmentTypeFlags == 5) || (m_segmentTypeFlags == 6) || (m_segmentTypeFlags == 10) || (m_segmentTypeFlags == 11)) { // undefined
        errorMessage = "error in NextStateAfterHeaderExtensions: undefined segment type flags: " + boost::lexical_cast<std::string>((int)m_segmentTypeFlags);
        return NULL;
    }
    else if (m_segmentTypeFlags <= 7) {
        m_sdnvTempVec.clear();
        m_dataSegmentRxState = LTP_DATA_SEGMENT_RX_STATE::READ_CLIENT_SERVICE_ID_SDNV;
        m_mainRxState = LTP_MAIN_RX_STATE::READ_DATA_SEGMENT_CONTENT;
        return Ltp::TryShortcutReadDataSegmentSdnvs(rxVals, numChars, errorMessage);
    }
    else if (m_segmentTypeFlags == 8) {
        m_sdnvTempVec.clear();
        m_reportSegmentRxState = LTP_REPORT_SEGMENT_RX_STATE::READ_REPORT_SERIAL_NUMBER_SDNV;
        m_mainRxState = LTP_MAIN_RX_STATE::READ_REPORT_SEGMENT_CONTENT;
        return Ltp::TryShortcutReadReportSegmentSdnvs(rxVals, numChars, errorMessage);
    }
    else if (m_segmentTypeFlags == 9) {
        m_sdnvTempVec.clear();
        m_mainRxState = LTP_MAIN_RX_STATE::READ_REPORT_ACKNOWLEDGEMENT_SEGMENT_CONTENT;
    }
    else { //12 or 14 => cancel segment
        m_mainRxState = LTP_MAIN_RX_STATE::READ_CANCEL_SEGMENT_CONTENT_BYTE;
    }
    return rxVals;
}

bool Ltp::NextStateAfterTrailerExtensions(std::string & errorMessage) {
    
    if (m_numTrailerExtensionTlvs) {
        //callback trailer
    }

    if ((m_segmentTypeFlags & 0xd) == 0xd) { //CAx (cancel ack) with no contents
        //callback cancel acknowledgement segment
        if (m_cancelAcknowledgementSegmentContentsReadCallback) {
            m_cancelAcknowledgementSegmentContentsReadCallback(m_sessionId, (m_segmentTypeFlags == (static_cast<uint8_t>(LTP_SEGMENT_TYPE_FLAGS::CANCEL_ACK_SEGMENT_TO_BLOCK_SENDER))), m_headerExtensions, m_trailerExtensions);
        }
    }
    else if ((m_segmentTypeFlags == 5) || (m_segmentTypeFlags == 6) || (m_segmentTypeFlags == 10) || (m_segmentTypeFlags == 11)) { // undefined
        errorMessage = "error in NextStateAfterTrailerExtensions: undefined segment type flags: " + boost::lexical_cast<std::string>((int)m_segmentTypeFlags);
        return false;
    }
    else if (m_segmentTypeFlags <= 7) {
        //callback data segment
        if (m_dataSegmentContentsReadCallback) {
            m_dataSegmentContentsReadCallback(m_segmentTypeFlags, m_sessionId, m_dataSegment_clientServiceData, m_dataSegmentMetadata, m_headerExtensions, m_trailerExtensions);
        }
    }
    else if (m_segmentTypeFlags == 8) {
        //callback report segment
        if (m_reportSegmentContentsReadCallback) {
            m_reportSegmentContentsReadCallback(m_sessionId, m_reportSegment, m_headerExtensions, m_trailerExtensions);
        }
    }
    else if (m_segmentTypeFlags == 9) {
        //callback report acknowledgement segment
        if (m_reportAcknowledgementSegmentContentsReadCallback) {
            m_reportAcknowledgementSegmentContentsReadCallback(m_sessionId, m_reportAcknowledgementSegment_reportSerialNumber, m_headerExtensions, m_trailerExtensions);
        }
    }
    else { //12 or 14 => cancel segment
        //callback cancel segment
        if (m_cancelSegmentContentsReadCallback) {
            m_cancelSegmentContentsReadCallback(m_sessionId, (static_cast<CANCEL_SEGMENT_REASON_CODES>(m_cancelSegment_reasonCode)), (m_segmentTypeFlags == (static_cast<uint8_t>(LTP_SEGMENT_TYPE_FLAGS::CANCEL_SEGMENT_FROM_BLOCK_SENDER))), m_headerExtensions, m_trailerExtensions);
        }
    }
    SetBeginningState();
    return true;
}

//Preconditions before call:
//m_sdnvTempVec.clear();
//m_dataSegmentRxState = LTP_DATA_SEGMENT_RX_STATE::READ_CLIENT_SERVICE_ID_SDNV;
//m_mainRxState = LTP_MAIN_RX_STATE::READ_DATA_SEGMENT_CONTENT;
//
//After call:
//Leaves m_dataSegmentRxState in proper state not exceeding last state READ_CLIENT_SERVICE_DATA
const uint8_t * Ltp::TryShortcutReadDataSegmentSdnvs(const uint8_t * rxVals, std::size_t & numChars, std::string & errorMessage) {

#if 1 //this is the sdnv array decode (batch operation) version
    static constexpr unsigned int maxNumSdnvsToDecode =
        1 + //m_dataSegmentMetadata.clientServiceId
        1 + //m_dataSegmentMetadata.offset
        1 + //m_dataSegmentMetadata.length
        1 + //m_dataSegmentMetadata.tmpCheckpointSerialNumber
        1; //m_dataSegmentMetadata.tmpReportSerialNumber
    uint64_t * const decodedSdnvs = &m_dataSegmentMetadata.clientServiceId; //start of the "array"
    static constexpr uint16_t CHECKPOINT_TYPE_MESSAGES =
        (1U << (static_cast<uint8_t>(LTP_SEGMENT_TYPE_FLAGS::REDDATA_CHECKPOINT))) |
        (1U << (static_cast<uint8_t>(LTP_SEGMENT_TYPE_FLAGS::REDDATA_CHECKPOINT_ENDOFREDPART))) |
        (1U << (static_cast<uint8_t>(LTP_SEGMENT_TYPE_FLAGS::REDDATA_CHECKPOINT_ENDOFREDPART_ENDOFBLOCK)));
    const uint16_t mask = (static_cast<uint16_t>(1)) << m_segmentTypeFlags;
    const bool isCheckPoint = ((mask & CHECKPOINT_TYPE_MESSAGES) != 0);
    static const uint8_t numSdnvsToDecodeByIsCheckpoint[2] = { 3,5 };
    const unsigned int numSdnvsToDecode = numSdnvsToDecodeByIsCheckpoint[isCheckPoint];
    uint64_t numBytesTakenToDecodeThisSdnvArray;
    bool decodeErrorDetected;
    const unsigned int numValuesActuallyDecoded = SdnvDecodeArrayU64(rxVals, numBytesTakenToDecodeThisSdnvArray, decodedSdnvs, numSdnvsToDecode, numChars, decodeErrorDetected);
    if (decodeErrorDetected) {
        return NULL; //failure
    }
    if (numValuesActuallyDecoded == 0) {
        return rxVals; //fall back to slower decode due to not enough bytes being available (leaving state unmodified)
    }

    //it turns out that the LTP_DATA_SEGMENT_RX_STATE will be identical to the numValuesActuallyDecoded so no LUT is needed
    m_dataSegmentRxState = static_cast<LTP_DATA_SEGMENT_RX_STATE>(numValuesActuallyDecoded);

    if (numValuesActuallyDecoded >= 3) { //we at least have length
        if (m_dataSegmentMetadata.length == 0) { //not sure if this is correct
            errorMessage = "error in shortcut LTP_DATA_SEGMENT_RX_STATE::READ_LENGTH_SDNV, length == 0";
            return NULL;
        }
        m_dataSegment_clientServiceData.clear();
        m_dataSegment_clientServiceData.reserve(m_dataSegmentMetadata.length); //todo make sure cant crash
        if (isCheckPoint) { //checkpoint
            m_dataSegmentMetadata.checkpointSerialNumber = &m_dataSegmentMetadata.tmpCheckpointSerialNumber;
            m_dataSegmentMetadata.reportSerialNumber = &m_dataSegmentMetadata.tmpReportSerialNumber;
        }
        else { //not checkpoint
            m_dataSegmentRxState = LTP_DATA_SEGMENT_RX_STATE::READ_CLIENT_SERVICE_DATA; //only case where m_dataSegmentRxState needs modified from above
            m_dataSegmentMetadata.checkpointSerialNumber = NULL;
            m_dataSegmentMetadata.reportSerialNumber = NULL;
        }
    }
    numChars -= numBytesTakenToDecodeThisSdnvArray;
    return (rxVals + numBytesTakenToDecodeThisSdnvArray);

#else //below is the working non-batch read version
    uint8_t sdnvSize;

    //shortcut READ_CLIENT_SERVICE_ID_SDNV
    if (numChars >= 16) { //shortcut/optimization to avoid reading populating m_sdnvTempVec, just decode from rxVals if there's enough bytes remaining 
        m_dataSegmentMetadata.clientServiceId = SdnvDecodeU64(rxVals, &sdnvSize, numChars);
        if (sdnvSize == 0) {
            errorMessage = "error in shortcut LTP_DATA_SEGMENT_RX_STATE::READ_CLIENT_SERVICE_ID_SDNV, sdnvSize is 0";
            return NULL;
        }
        else { //success READ_CLIENT_SERVICE_ID_SDNV 
            numChars -= sdnvSize;
            rxVals += sdnvSize;
            m_dataSegmentRxState = LTP_DATA_SEGMENT_RX_STATE::READ_OFFSET_SDNV;
        }
    }
    else { //not enough bytes, populate m_sdnvTempVec and then decode sdnv
        return rxVals;
    }

    //shortcut READ_OFFSET_SDNV
    if (numChars >= 16) { //shortcut/optimization to avoid reading populating m_sdnvTempVec, just decode from rxVals if there's enough bytes remaining
        m_dataSegmentMetadata.offset = SdnvDecodeU64(rxVals, &sdnvSize, numChars);
        if (sdnvSize == 0) {
            errorMessage = "error in shortcut LTP_DATA_SEGMENT_RX_STATE::READ_OFFSET_SDNV, sdnvSize is 0";
            return NULL;
        }
        else { //success READ_OFFSET_SDNV 
            numChars -= sdnvSize;
            rxVals += sdnvSize;
            m_dataSegmentRxState = LTP_DATA_SEGMENT_RX_STATE::READ_LENGTH_SDNV;
        }
    }
    else { //not enough bytes, populate m_sdnvTempVec and then decode sdnv
        return rxVals;
    }

    //shortcut READ_LENGTH_SDNV
    if (numChars >= 16) { //shortcut/optimization to avoid reading populating m_sdnvTempVec, just decode from rxVals if there's enough bytes remaining
        m_dataSegmentMetadata.length = SdnvDecodeU64(rxVals, &sdnvSize, numChars);
        if (sdnvSize == 0) {
            errorMessage = "error in shortcut LTP_DATA_SEGMENT_RX_STATE::READ_LENGTH_SDNV, sdnvSize is 0";
            return NULL;
        }
        else if (m_dataSegmentMetadata.length == 0) { //not sure if this is correct
            errorMessage = "error in shortcut LTP_DATA_SEGMENT_RX_STATE::READ_LENGTH_SDNV, length == 0";
            return NULL;
        }
        else { //success READ_LENGTH_SDNV 
            numChars -= sdnvSize;
            rxVals += sdnvSize;
            if ((m_segmentTypeFlags >= 1) && (m_segmentTypeFlags <= 3)) { //checkpoint
                m_dataSegmentRxState = LTP_DATA_SEGMENT_RX_STATE::READ_CHECKPOINT_SERIAL_NUMBER_SDNV;
                m_dataSegmentMetadata.checkpointSerialNumber = &m_dataSegmentMetadata.tmpCheckpointSerialNumber;
                m_dataSegmentMetadata.reportSerialNumber = &m_dataSegmentMetadata.tmpReportSerialNumber;
            }
            else {
                m_dataSegment_clientServiceData.clear();
                m_dataSegment_clientServiceData.reserve(m_dataSegmentMetadata.length); //todo make sure cant crash
                m_dataSegmentRxState = LTP_DATA_SEGMENT_RX_STATE::READ_CLIENT_SERVICE_DATA;
                m_dataSegmentMetadata.checkpointSerialNumber = NULL;
                m_dataSegmentMetadata.reportSerialNumber = NULL;
                return rxVals;
            }

        }
    }
    else { //not enough bytes, populate m_sdnvTempVec and then decode sdnv
        return rxVals;
    }

    //shortcut READ_CHECKPOINT_SERIAL_NUMBER_SDNV
    if (numChars >= 16) { //shortcut/optimization to avoid reading populating m_sdnvTempVec, just decode from rxVals if there's enough bytes remaining
        m_dataSegmentMetadata.tmpCheckpointSerialNumber = SdnvDecodeU64(rxVals, &sdnvSize, numChars);
        if (sdnvSize == 0) {
            errorMessage = "error in shortcut LTP_DATA_SEGMENT_RX_STATE::READ_CHECKPOINT_SERIAL_NUMBER_SDNV, sdnvSize is 0";
            return NULL;
        }
        else { //success READ_CHECKPOINT_SERIAL_NUMBER_SDNV 
            numChars -= sdnvSize;
            rxVals += sdnvSize;
            m_dataSegmentRxState = LTP_DATA_SEGMENT_RX_STATE::READ_REPORT_SERIAL_NUMBER_SDNV;
        }
    }
    else { //not enough bytes, populate m_sdnvTempVec and then decode sdnv
        return rxVals;
    }

    //shortcut READ_REPORT_SERIAL_NUMBER_SDNV
    if (numChars >= 16) { //shortcut/optimization to avoid reading populating m_sdnvTempVec, just decode from rxVals if there's enough bytes remaining
        m_dataSegmentMetadata.tmpReportSerialNumber = SdnvDecodeU64(rxVals, &sdnvSize, numChars);
        if (sdnvSize == 0) {
            errorMessage = "error in shortcut LTP_DATA_SEGMENT_RX_STATE::READ_REPORT_SERIAL_NUMBER_SDNV, sdnvSize is 0";
            return NULL;
        }
        else { //success READ_REPORT_SERIAL_NUMBER_SDNV 
            numChars -= sdnvSize;
            rxVals += sdnvSize;
            m_dataSegment_clientServiceData.clear();
            m_dataSegment_clientServiceData.reserve(m_dataSegmentMetadata.length); //todo make sure cant crash
            m_dataSegmentRxState = LTP_DATA_SEGMENT_RX_STATE::READ_CLIENT_SERVICE_DATA;
            return rxVals;
        }
    }
    else { //not enough bytes, populate m_sdnvTempVec and then decode sdnv
        return rxVals;
    }
#endif
}

//Preconditions before call:
//m_sdnvTempVec.clear();
//m_reportSegmentRxState = LTP_REPORT_SEGMENT_RX_STATE::READ_REPORT_SERIAL_NUMBER_SDNV;
//m_mainRxState = LTP_MAIN_RX_STATE::READ_REPORT_SEGMENT_CONTENT;
const uint8_t * Ltp::TryShortcutReadReportSegmentSdnvs(const uint8_t * rxVals, std::size_t & numChars, std::string & errorMessage) {
#if 1 //this is the sdnv array decode (batch operation) version
    static constexpr unsigned int numSdnvsToDecode =
        1 + //m_reportSegment.reportSerialNumber
        1 + //m_reportSegment.checkpointSerialNumber
        1 + //m_reportSegment.upperBound
        1 + //m_reportSegment.lowerBound
        1; //m_reportSegment.tmpReceptionClaimCount
    uint64_t * const decodedSdnvs = &m_reportSegment.reportSerialNumber; //start of the "array"
    uint64_t numBytesTakenToDecodeThisSdnvArray;
    bool decodeErrorDetected;
    const unsigned int numValuesActuallyDecoded = SdnvDecodeArrayU64(rxVals, numBytesTakenToDecodeThisSdnvArray, decodedSdnvs, numSdnvsToDecode, numChars, decodeErrorDetected);
    if (decodeErrorDetected) {
        return NULL; //failure
    }
    if (numValuesActuallyDecoded == 0) {
        return rxVals; //fall back to slower decode due to not enough bytes being available (leaving state unmodified)
    }

    //it turns out that the LTP_REPORT_SEGMENT_RX_STATE will be identical to the numValuesActuallyDecoded so no LUT is needed
    m_reportSegmentRxState = static_cast<LTP_REPORT_SEGMENT_RX_STATE>(numValuesActuallyDecoded);

    numChars -= numBytesTakenToDecodeThisSdnvArray;
    rxVals += numBytesTakenToDecodeThisSdnvArray;
    if (numValuesActuallyDecoded < numSdnvsToDecode) {
        return rxVals;
    }

    //at this point (numValuesActuallyDecoded == numSdnvsToDecode), so we at least have receptionClaimCount (current state is LTP_REPORT_SEGMENT_RX_STATE::READ_ONE_RECEPTION_CLAIM_OFFSET_SDNV
    if (m_reportSegment.tmpReceptionClaimCount == 0) { //must be 1 or more (The content of an RS comprises one or more data reception claims)
        errorMessage = "error in shortcut LTP_REPORT_SEGMENT_RX_STATE::READ_RECEPTION_CLAIM_COUNT_SDNV, count == 0";
        return NULL;
    }
    m_reportSegment.receptionClaims.clear();
    m_reportSegment.receptionClaims.resize(m_reportSegment.tmpReceptionClaimCount); //must resize instead of reserve or default ctor will be called.. resize later
    const unsigned int numReceptionClaimSdnvsToDecode = static_cast<unsigned int>(m_reportSegment.tmpReceptionClaimCount << 1); //*2
    const unsigned int numReceptionClaimSdnvsActuallyDecoded = SdnvDecodeArrayU64(rxVals, numBytesTakenToDecodeThisSdnvArray,
        (uint64_t *)m_reportSegment.receptionClaims.data(), numReceptionClaimSdnvsToDecode, numChars, decodeErrorDetected);
    if (decodeErrorDetected) {
        return NULL; //failure
    }
    //if odd then only read offset, need to read length, but if even then read length but may need to loop back to beginning and read another offset
    const unsigned int isOddAsInt = (numReceptionClaimSdnvsActuallyDecoded & 1u); 
    //READ_ONE_RECEPTION_CLAIM_OFFSET_SDNV = 5, READ_ONE_RECEPTION_CLAIM_LENGTH_SDNV = 6
    m_reportSegmentRxState = static_cast<LTP_REPORT_SEGMENT_RX_STATE>(static_cast<unsigned int>(LTP_REPORT_SEGMENT_RX_STATE::READ_ONE_RECEPTION_CLAIM_OFFSET_SDNV) + isOddAsInt); //set next state
    m_reportSegment.receptionClaims.resize((numReceptionClaimSdnvsActuallyDecoded + 1) >> 1); //ceil(numReceptionClaimSdnvsActuallyDecoded/2.0)
    const unsigned int numLengthsRead = numReceptionClaimSdnvsActuallyDecoded >> 1; //floor(numReceptionClaimSdnvsActuallyDecoded/2.0) since offset read first then length second
    for (unsigned int i = 0; i < numLengthsRead; ++i) {
        if (m_reportSegment.receptionClaims[i].length == 0) {
            //must be 1 or more (A reception claim's length shall never be less than 1)
            errorMessage = "error in shortcut LTP_REPORT_SEGMENT_RX_STATE::READ_ONE_RECEPTION_CLAIM_LENGTH_SDNV, count == 0";
            return NULL;
        }
    }
    if (numReceptionClaimSdnvsToDecode == numReceptionClaimSdnvsActuallyDecoded) { //done
        if (m_numTrailerExtensionTlvs) {
            m_trailerRxState = LTP_TRAILER_RX_STATE::READ_ONE_TRAILER_EXTENSION_TAG_BYTE;
            m_mainRxState = LTP_MAIN_RX_STATE::READ_TRAILER;
        }
        else {
            //callback report segment
            if (m_reportSegmentContentsReadCallback) {
                m_reportSegmentContentsReadCallback(m_sessionId, m_reportSegment, m_headerExtensions, m_trailerExtensions);
            }
            SetBeginningState();
        }
    }
    
    numChars -= numBytesTakenToDecodeThisSdnvArray;
    rxVals += numBytesTakenToDecodeThisSdnvArray;
    return rxVals;

#else //below is the working non-batch read version
    uint8_t sdnvSize;

    //shortcut READ_REPORT_SERIAL_NUMBER_SDNV
    if (numChars >= 16) { //shortcut/optimization to avoid reading populating m_sdnvTempVec, just decode from rxVals if there's enough bytes remaining 
        m_reportSegment.reportSerialNumber = SdnvDecodeU64(rxVals, &sdnvSize, numChars);
        if (sdnvSize == 0) {
            errorMessage = "error in shortcut LTP_REPORT_SEGMENT_RX_STATE::READ_REPORT_SERIAL_NUMBER_SDNV, sdnvSize is 0";
            return NULL;
        }
        else { //success READ_REPORT_SERIAL_NUMBER_SDNV 
            numChars -= sdnvSize;
            rxVals += sdnvSize;
            m_reportSegmentRxState = LTP_REPORT_SEGMENT_RX_STATE::READ_CHECKPOINT_SERIAL_NUMBER_SDNV;
        }
    }
    else { //not enough bytes, populate m_sdnvTempVec and then decode sdnv
        return rxVals;
    }

    //shortcut READ_CHECKPOINT_SERIAL_NUMBER_SDNV
    if (numChars >= 16) { //shortcut/optimization to avoid reading populating m_sdnvTempVec, just decode from rxVals if there's enough bytes remaining 
        m_reportSegment.checkpointSerialNumber = SdnvDecodeU64(rxVals, &sdnvSize, numChars);
        if (sdnvSize == 0) {
            errorMessage = "error in shortcut LTP_REPORT_SEGMENT_RX_STATE::READ_CHECKPOINT_SERIAL_NUMBER_SDNV, sdnvSize is 0";
            return NULL;
        }
        else { //success READ_CHECKPOINT_SERIAL_NUMBER_SDNV 
            numChars -= sdnvSize;
            rxVals += sdnvSize;
            m_reportSegmentRxState = LTP_REPORT_SEGMENT_RX_STATE::READ_UPPER_BOUND_SDNV;
        }
    }
    else { //not enough bytes, populate m_sdnvTempVec and then decode sdnv
        return rxVals;
    }

    //shortcut READ_UPPER_BOUND_SDNV
    if (numChars >= 16) { //shortcut/optimization to avoid reading populating m_sdnvTempVec, just decode from rxVals if there's enough bytes remaining 
        m_reportSegment.upperBound = SdnvDecodeU64(rxVals, &sdnvSize, numChars);
        if (sdnvSize == 0) {
            errorMessage = "error in shortcut LTP_REPORT_SEGMENT_RX_STATE::READ_UPPER_BOUND_SDNV, sdnvSize is 0";
            return NULL;
        }
        else { //success READ_UPPER_BOUND_SDNV 
            numChars -= sdnvSize;
            rxVals += sdnvSize;
            m_reportSegmentRxState = LTP_REPORT_SEGMENT_RX_STATE::READ_LOWER_BOUND_SDNV;
        }
    }
    else { //not enough bytes, populate m_sdnvTempVec and then decode sdnv
        return rxVals;
    }

    //shortcut READ_LOWER_BOUND_SDNV
    if (numChars >= 16) { //shortcut/optimization to avoid reading populating m_sdnvTempVec, just decode from rxVals if there's enough bytes remaining 
        m_reportSegment.lowerBound = SdnvDecodeU64(rxVals, &sdnvSize, numChars);
        if (sdnvSize == 0) {
            errorMessage = "error in shortcut LTP_REPORT_SEGMENT_RX_STATE::READ_LOWER_BOUND_SDNV, sdnvSize is 0";
            return NULL;
        }
        else { //success READ_LOWER_BOUND_SDNV 
            numChars -= sdnvSize;
            rxVals += sdnvSize;
            m_reportSegmentRxState = LTP_REPORT_SEGMENT_RX_STATE::READ_RECEPTION_CLAIM_COUNT_SDNV;
        }
    }
    else { //not enough bytes, populate m_sdnvTempVec and then decode sdnv
        return rxVals;
    }

    //shortcut READ_RECEPTION_CLAIM_COUNT_SDNV
    if (numChars >= 16) { //shortcut/optimization to avoid reading populating m_sdnvTempVec, just decode from rxVals if there's enough bytes remaining 
        m_reportSegment.tmpReceptionClaimCount = SdnvDecodeU64(rxVals, &sdnvSize, numChars);
        if (sdnvSize == 0) {
            errorMessage = "error in shortcut LTP_REPORT_SEGMENT_RX_STATE::READ_RECEPTION_CLAIM_COUNT_SDNV, sdnvSize is 0";
            return NULL;
        }
        else { //success READ_RECEPTION_CLAIM_COUNT_SDNV 
            numChars -= sdnvSize;
            rxVals += sdnvSize;
            if (m_reportSegment.tmpReceptionClaimCount == 0) { //must be 1 or more (The content of an RS comprises one or more data reception claims)
                errorMessage = "error in shortcut LTP_REPORT_SEGMENT_RX_STATE::READ_RECEPTION_CLAIM_COUNT_SDNV, count == 0";
                return NULL;
            }
            else {
                m_reportSegment.receptionClaims.clear();
                m_reportSegment.receptionClaims.reserve(m_reportSegment.tmpReceptionClaimCount);
                m_reportSegmentRxState = LTP_REPORT_SEGMENT_RX_STATE::READ_ONE_RECEPTION_CLAIM_OFFSET_SDNV;
            }
        }
    }
    else { //not enough bytes, populate m_sdnvTempVec and then decode sdnv
        return rxVals;
    }

    while (true) {
        //shortcut READ_ONE_RECEPTION_CLAIM_OFFSET_SDNV
        if (numChars >= 16) { //shortcut/optimization to avoid reading populating m_sdnvTempVec, just decode from rxVals if there's enough bytes remaining 
            const uint64_t claimOffset = SdnvDecodeU64(rxVals, &sdnvSize, numChars);
            if (sdnvSize == 0) {
                errorMessage = "error in shortcut LTP_REPORT_SEGMENT_RX_STATE::READ_ONE_RECEPTION_CLAIM_OFFSET_SDNV, sdnvSize is 0";
                return NULL;
            }
            else { //success READ_ONE_RECEPTION_CLAIM_OFFSET_SDNV 
                numChars -= sdnvSize;
                rxVals += sdnvSize;
                m_reportSegment.receptionClaims.resize(m_reportSegment.receptionClaims.size() + 1);
                m_reportSegment.receptionClaims.back().offset = claimOffset;
                m_reportSegmentRxState = LTP_REPORT_SEGMENT_RX_STATE::READ_ONE_RECEPTION_CLAIM_LENGTH_SDNV;
            }
        }
        else { //not enough bytes, populate m_sdnvTempVec and then decode sdnv
            return rxVals;
        }

        //shortcut READ_ONE_RECEPTION_CLAIM_LENGTH_SDNV
        if (numChars >= 16) { //shortcut/optimization to avoid reading populating m_sdnvTempVec, just decode from rxVals if there's enough bytes remaining 
            const uint64_t claimLength = SdnvDecodeU64(rxVals, &sdnvSize, numChars);
            if (sdnvSize == 0) {
                errorMessage = "error in shortcut LTP_REPORT_SEGMENT_RX_STATE::READ_ONE_RECEPTION_CLAIM_LENGTH_SDNV, sdnvSize is 0";
                return NULL;
            }
            else { //success READ_ONE_RECEPTION_CLAIM_LENGTH_SDNV 
                numChars -= sdnvSize;
                rxVals += sdnvSize;
                if (claimLength == 0) { //must be 1 or more (A reception claim's length shall never be less than 1)
                    errorMessage = "error in shortcut LTP_REPORT_SEGMENT_RX_STATE::READ_ONE_RECEPTION_CLAIM_LENGTH_SDNV, count == 0";
                    return NULL;
                }
                else {
                    m_reportSegment.receptionClaims.back().length = claimLength;
                    if (m_reportSegment.receptionClaims.size() < m_reportSegment.tmpReceptionClaimCount) {
                        m_reportSegmentRxState = LTP_REPORT_SEGMENT_RX_STATE::READ_ONE_RECEPTION_CLAIM_OFFSET_SDNV;
                    }
                    else if (m_numTrailerExtensionTlvs) {
                        m_trailerRxState = LTP_TRAILER_RX_STATE::READ_ONE_TRAILER_EXTENSION_TAG_BYTE;
                        m_mainRxState = LTP_MAIN_RX_STATE::READ_TRAILER;
                        return rxVals;
                    }
                    else {
                        //callback report segment
                        if (m_reportSegmentContentsReadCallback) {
                            m_reportSegmentContentsReadCallback(m_sessionId, m_reportSegment, m_headerExtensions, m_trailerExtensions);
                        }
                        SetBeginningState();
                        return rxVals;
                    }
                }
            }
        }
        else { //not enough bytes, populate m_sdnvTempVec and then decode sdnv
            return rxVals;
        }
    }
#endif
}





void Ltp::GenerateLtpHeaderPlusDataSegmentMetadata(std::vector<uint8_t> & ltpHeaderPlusDataSegmentMetadata, LTP_DATA_SEGMENT_TYPE_FLAGS dataSegmentTypeFlags, 
    const session_id_t & sessionId, const data_segment_metadata_t & dataSegmentMetadata,
    ltp_extensions_t * headerExtensions, uint8_t numTrailerExtensions)
{
    uint8_t numHeaderExtensions = 0;
    uint64_t maxBytesRequiredForHeaderExtensions = 0;
    if (headerExtensions) {
        numHeaderExtensions = static_cast<uint8_t>(headerExtensions->extensionsVec.size());
        maxBytesRequiredForHeaderExtensions = headerExtensions->GetMaximumDataRequiredForSerialization();
    }
    ltpHeaderPlusDataSegmentMetadata.resize(1 + 1 + (2 * 10) + dataSegmentMetadata.GetMaximumDataRequiredForSerialization() + maxBytesRequiredForHeaderExtensions); //flags + extensionCounts + 2 10-byte session sdnvs + metadata sdnvs + header extensions
    uint8_t * encodedPtr = ltpHeaderPlusDataSegmentMetadata.data();
    *encodedPtr++ = static_cast<uint8_t>(dataSegmentTypeFlags); //assumes version 0 in most significant 4 bits
    encodedPtr += SdnvEncodeU64BufSize10(encodedPtr, sessionId.sessionOriginatorEngineId);
    encodedPtr += SdnvEncodeU64BufSize10(encodedPtr, sessionId.sessionNumber);
    *encodedPtr++ = (numHeaderExtensions << 4) | numTrailerExtensions;
    if (headerExtensions) {
        encodedPtr += headerExtensions->Serialize(encodedPtr);
    }
    encodedPtr += dataSegmentMetadata.Serialize(encodedPtr);
    ltpHeaderPlusDataSegmentMetadata.resize(encodedPtr - ltpHeaderPlusDataSegmentMetadata.data());
}

void Ltp::GenerateReportSegmentLtpPacket(std::vector<uint8_t> & ltpReportSegmentPacket, const session_id_t & sessionId, const report_segment_t & reportSegmentStruct,
    ltp_extensions_t * headerExtensions, ltp_extensions_t * trailerExtensions)
{
    uint8_t numHeaderExtensions = 0;
    uint64_t maxBytesRequiredForHeaderExtensions = 0;
    if (headerExtensions) {
        numHeaderExtensions = static_cast<uint8_t>(headerExtensions->extensionsVec.size());
        maxBytesRequiredForHeaderExtensions = headerExtensions->GetMaximumDataRequiredForSerialization();
    }
    uint8_t numTrailerExtensions = 0;
    uint64_t maxBytesRequiredForTrailerExtensions = 0;
    if (trailerExtensions) {
        numTrailerExtensions = static_cast<uint8_t>(trailerExtensions->extensionsVec.size());
        maxBytesRequiredForTrailerExtensions = trailerExtensions->GetMaximumDataRequiredForSerialization();
    }
    ltpReportSegmentPacket.resize(1 + 1 + (2 * 10) + reportSegmentStruct.GetMaximumDataRequiredForSerialization() + maxBytesRequiredForHeaderExtensions + maxBytesRequiredForTrailerExtensions); //flags + extensionCounts + 2 session 10-byte sdnvs + rest of data
    uint8_t * encodedPtr = ltpReportSegmentPacket.data();
    *encodedPtr++ = static_cast<uint8_t>(LTP_SEGMENT_TYPE_FLAGS::REPORT_SEGMENT); //assumes version 0 in most significant 4 bits
    encodedPtr += SdnvEncodeU64BufSize10(encodedPtr, sessionId.sessionOriginatorEngineId);
    encodedPtr += SdnvEncodeU64BufSize10(encodedPtr, sessionId.sessionNumber);
    *encodedPtr++ = (numHeaderExtensions << 4) | numTrailerExtensions;
    if (headerExtensions) {
        encodedPtr += headerExtensions->Serialize(encodedPtr);
    }
    encodedPtr += reportSegmentStruct.Serialize(encodedPtr);
    if (trailerExtensions) {
        encodedPtr += trailerExtensions->Serialize(encodedPtr);
    }
    ltpReportSegmentPacket.resize(encodedPtr - ltpReportSegmentPacket.data());
}

void Ltp::GenerateReportAcknowledgementSegmentLtpPacket(std::vector<uint8_t> & ltpReportAcknowledgementSegmentPacket, const session_id_t & sessionId,
    uint64_t reportSerialNumberBeingAcknowledged, ltp_extensions_t * headerExtensions, ltp_extensions_t * trailerExtensions)
{
    uint8_t numHeaderExtensions = 0;
    uint64_t maxBytesRequiredForHeaderExtensions = 0;
    if (headerExtensions) {
        numHeaderExtensions = static_cast<uint8_t>(headerExtensions->extensionsVec.size());
        maxBytesRequiredForHeaderExtensions = headerExtensions->GetMaximumDataRequiredForSerialization();
    }
    uint8_t numTrailerExtensions = 0;
    uint64_t maxBytesRequiredForTrailerExtensions = 0;
    if (trailerExtensions) {
        numTrailerExtensions = static_cast<uint8_t>(trailerExtensions->extensionsVec.size());
        maxBytesRequiredForTrailerExtensions = trailerExtensions->GetMaximumDataRequiredForSerialization();
    }
    ltpReportAcknowledgementSegmentPacket.resize(1 + 1 + (2 * 10) + (1 * 10) + maxBytesRequiredForHeaderExtensions + maxBytesRequiredForTrailerExtensions); //flags + extensionCounts + 2 session 10-byte sdnvs + 1 report serial number 10-byte sdnv + rest of data
    uint8_t * encodedPtr = ltpReportAcknowledgementSegmentPacket.data();
    *encodedPtr++ = static_cast<uint8_t>(LTP_SEGMENT_TYPE_FLAGS::REPORT_ACK_SEGMENT); //assumes version 0 in most significant 4 bits
    encodedPtr += SdnvEncodeU64BufSize10(encodedPtr, sessionId.sessionOriginatorEngineId);
    encodedPtr += SdnvEncodeU64BufSize10(encodedPtr, sessionId.sessionNumber);
    *encodedPtr++ = (numHeaderExtensions << 4) | numTrailerExtensions;
    if (headerExtensions) {
        encodedPtr += headerExtensions->Serialize(encodedPtr);
    }
    encodedPtr += SdnvEncodeU64BufSize10(encodedPtr, reportSerialNumberBeingAcknowledged);
    if (trailerExtensions) {
        encodedPtr += trailerExtensions->Serialize(encodedPtr);
    }
    ltpReportAcknowledgementSegmentPacket.resize(encodedPtr - ltpReportAcknowledgementSegmentPacket.data());
}

void Ltp::GenerateCancelSegmentLtpPacket(std::vector<uint8_t> & ltpCancelSegmentPacket, const session_id_t & sessionId, CANCEL_SEGMENT_REASON_CODES reasonCode,
    bool isFromSender, ltp_extensions_t * headerExtensions, ltp_extensions_t * trailerExtensions)
{
    uint8_t numHeaderExtensions = 0;
    uint64_t maxBytesRequiredForHeaderExtensions = 0;
    if (headerExtensions) {
        numHeaderExtensions = static_cast<uint8_t>(headerExtensions->extensionsVec.size());
        maxBytesRequiredForHeaderExtensions = headerExtensions->GetMaximumDataRequiredForSerialization();
    }
    uint8_t numTrailerExtensions = 0;
    uint64_t maxBytesRequiredForTrailerExtensions = 0;
    if (trailerExtensions) {
        numTrailerExtensions = static_cast<uint8_t>(trailerExtensions->extensionsVec.size());
        maxBytesRequiredForTrailerExtensions = trailerExtensions->GetMaximumDataRequiredForSerialization();
    }
    ltpCancelSegmentPacket.resize(1 + 1 + (2 * 10) + 1 + maxBytesRequiredForHeaderExtensions + maxBytesRequiredForTrailerExtensions); //flags + extensionCounts + 2 session 10-byte sdnvs + 1 one-byte reason code + rest of data
    uint8_t * encodedPtr = ltpCancelSegmentPacket.data();
    *encodedPtr++ = static_cast<uint8_t>(isFromSender ? LTP_SEGMENT_TYPE_FLAGS::CANCEL_SEGMENT_FROM_BLOCK_SENDER : LTP_SEGMENT_TYPE_FLAGS::CANCEL_SEGMENT_FROM_BLOCK_RECEIVER); //assumes version 0 in most significant 4 bits
    encodedPtr += SdnvEncodeU64BufSize10(encodedPtr, sessionId.sessionOriginatorEngineId);
    encodedPtr += SdnvEncodeU64BufSize10(encodedPtr, sessionId.sessionNumber);
    *encodedPtr++ = (numHeaderExtensions << 4) | numTrailerExtensions;
    if (headerExtensions) {
        encodedPtr += headerExtensions->Serialize(encodedPtr);
    }
    *encodedPtr++ = static_cast<uint8_t>(reasonCode);
    if (trailerExtensions) {
        encodedPtr += trailerExtensions->Serialize(encodedPtr);
    }
    ltpCancelSegmentPacket.resize(encodedPtr - ltpCancelSegmentPacket.data());
}

void Ltp::GenerateCancelAcknowledgementSegmentLtpPacket(std::vector<uint8_t> & ltpCancelAcknowledgementSegmentPacket, const session_id_t & sessionId,
    bool isToSender, ltp_extensions_t * headerExtensions, ltp_extensions_t * trailerExtensions)
{
    uint8_t numHeaderExtensions = 0;
    uint64_t maxBytesRequiredForHeaderExtensions = 0;
    if (headerExtensions) {
        numHeaderExtensions = static_cast<uint8_t>(headerExtensions->extensionsVec.size());
        maxBytesRequiredForHeaderExtensions = headerExtensions->GetMaximumDataRequiredForSerialization();
    }
    uint8_t numTrailerExtensions = 0;
    uint64_t maxBytesRequiredForTrailerExtensions = 0;
    if (trailerExtensions) {
        numTrailerExtensions = static_cast<uint8_t>(trailerExtensions->extensionsVec.size());
        maxBytesRequiredForTrailerExtensions = trailerExtensions->GetMaximumDataRequiredForSerialization();
    }
    ltpCancelAcknowledgementSegmentPacket.resize(1 + 1 + (2 * 10) + 0 + maxBytesRequiredForHeaderExtensions + maxBytesRequiredForTrailerExtensions); //flags + extensionCounts + 2 session 10-byte sdnvs + no payload data + rest of data
    uint8_t * encodedPtr = ltpCancelAcknowledgementSegmentPacket.data();
    *encodedPtr++ = static_cast<uint8_t>(isToSender ? LTP_SEGMENT_TYPE_FLAGS::CANCEL_ACK_SEGMENT_TO_BLOCK_SENDER : LTP_SEGMENT_TYPE_FLAGS::CANCEL_ACK_SEGMENT_TO_BLOCK_RECEIVER); //assumes version 0 in most significant 4 bits
    encodedPtr += SdnvEncodeU64BufSize10(encodedPtr, sessionId.sessionOriginatorEngineId);
    encodedPtr += SdnvEncodeU64BufSize10(encodedPtr, sessionId.sessionNumber);
    *encodedPtr++ = (numHeaderExtensions << 4) | numTrailerExtensions;
    if (headerExtensions) {
        encodedPtr += headerExtensions->Serialize(encodedPtr);
    }
    if (trailerExtensions) {
        encodedPtr += trailerExtensions->Serialize(encodedPtr);
    }
    ltpCancelAcknowledgementSegmentPacket.resize(encodedPtr - ltpCancelAcknowledgementSegmentPacket.data());
}

//return true if valid message
bool Ltp::GetMessageDirectionFromSegmentFlags(const uint8_t segmentFlags, bool & isSenderToReceiver) {
#if 0
    switch (static_cast<LTP_SEGMENT_TYPE_FLAGS>(segmentFlags)) {
        case LTP_SEGMENT_TYPE_FLAGS::REDDATA:
        case LTP_SEGMENT_TYPE_FLAGS::REDDATA_CHECKPOINT:
        case LTP_SEGMENT_TYPE_FLAGS::REDDATA_CHECKPOINT_ENDOFREDPART:
        case LTP_SEGMENT_TYPE_FLAGS::REDDATA_CHECKPOINT_ENDOFREDPART_ENDOFBLOCK:
        case LTP_SEGMENT_TYPE_FLAGS::GREENDATA:
        case LTP_SEGMENT_TYPE_FLAGS::GREENDATA_ENDOFBLOCK:
        case LTP_SEGMENT_TYPE_FLAGS::REPORT_ACK_SEGMENT:
        case LTP_SEGMENT_TYPE_FLAGS::CANCEL_SEGMENT_FROM_BLOCK_SENDER:
        case LTP_SEGMENT_TYPE_FLAGS::CANCEL_ACK_SEGMENT_TO_BLOCK_RECEIVER:
            isSenderToReceiver = true;
            return true;
        case LTP_SEGMENT_TYPE_FLAGS::REPORT_SEGMENT:
        case LTP_SEGMENT_TYPE_FLAGS::CANCEL_ACK_SEGMENT_TO_BLOCK_SENDER:
        case LTP_SEGMENT_TYPE_FLAGS::CANCEL_SEGMENT_FROM_BLOCK_RECEIVER:
            isSenderToReceiver = false;
            return true;
    }
    return false;
#else
    //branchless routine
    static constexpr uint16_t SENDER_TO_RECEIVER_TYPE_MESSAGES =
        (1U << (static_cast<uint8_t>(LTP_SEGMENT_TYPE_FLAGS::REDDATA))) |
        (1U << (static_cast<uint8_t>(LTP_SEGMENT_TYPE_FLAGS::REDDATA_CHECKPOINT))) |
        (1U << (static_cast<uint8_t>(LTP_SEGMENT_TYPE_FLAGS::REDDATA_CHECKPOINT_ENDOFREDPART))) |
        (1U << (static_cast<uint8_t>(LTP_SEGMENT_TYPE_FLAGS::REDDATA_CHECKPOINT_ENDOFREDPART_ENDOFBLOCK))) |
        (1U << (static_cast<uint8_t>(LTP_SEGMENT_TYPE_FLAGS::GREENDATA))) |
        (1U << (static_cast<uint8_t>(LTP_SEGMENT_TYPE_FLAGS::GREENDATA_ENDOFBLOCK))) |
        (1U << (static_cast<uint8_t>(LTP_SEGMENT_TYPE_FLAGS::REPORT_ACK_SEGMENT))) |
        (1U << (static_cast<uint8_t>(LTP_SEGMENT_TYPE_FLAGS::CANCEL_SEGMENT_FROM_BLOCK_SENDER))) |
        (1U << (static_cast<uint8_t>(LTP_SEGMENT_TYPE_FLAGS::CANCEL_ACK_SEGMENT_TO_BLOCK_RECEIVER)));

    static constexpr uint16_t ALL_VALID_MESSAGES = SENDER_TO_RECEIVER_TYPE_MESSAGES |
        (1U << (static_cast<uint8_t>(LTP_SEGMENT_TYPE_FLAGS::REPORT_SEGMENT))) |
        (1U << (static_cast<uint8_t>(LTP_SEGMENT_TYPE_FLAGS::CANCEL_ACK_SEGMENT_TO_BLOCK_SENDER))) |
        (1U << (static_cast<uint8_t>(LTP_SEGMENT_TYPE_FLAGS::CANCEL_SEGMENT_FROM_BLOCK_RECEIVER)));

    const uint16_t mask = (static_cast<uint16_t>(1)) << segmentFlags;
    isSenderToReceiver = ((mask & SENDER_TO_RECEIVER_TYPE_MESSAGES) != 0);
    return ((mask & ALL_VALID_MESSAGES) != 0);
#endif
}
