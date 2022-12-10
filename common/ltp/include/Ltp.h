/**
 * @file Ltp.h
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
 * This Ltp class defines data structures used in the LTP library,
 * defines methods for encoding LTP headers (segments),
 * and defines methods for finite-state machine (FSM) receiving
 * of all bytes or partial bytes of an LTP message with custom callback functions
 * that (if the function is defined) get called whenever the appropriate number of bytes is received.
 */

#ifndef LTP_H
#define LTP_H 1
#include "ltp_lib_export.h"
#ifndef CLASS_VISIBILITY_LTP_LIB
#  ifdef _WIN32
#    define CLASS_VISIBILITY_LTP_LIB
#  else
#    define CLASS_VISIBILITY_LTP_LIB LTP_LIB_EXPORT
#  endif
#endif
#include <string>
#include <memory>
#include <boost/integer.hpp>
#include <boost/function.hpp>
#include <vector>

enum class LTP_MAIN_RX_STATE
{
	READ_HEADER = 0,
	READ_DATA_SEGMENT_CONTENT,
    READ_REPORT_SEGMENT_CONTENT,
    READ_REPORT_ACKNOWLEDGEMENT_SEGMENT_CONTENT,
    READ_CANCEL_SEGMENT_CONTENT_BYTE,
	READ_TRAILER
};

enum class LTP_HEADER_RX_STATE
{
	READ_CONTROL_BYTE = 0,
	READ_SESSION_ORIGINATOR_ENGINE_ID_SDNV = 1,
    READ_SESSION_NUMBER_SDNV = 2,
	READ_NUM_EXTENSIONS_BYTE = 3,
	READ_ONE_HEADER_EXTENSION_TAG_BYTE = 4,
    READ_ONE_HEADER_EXTENSION_LENGTH_SDNV = 5,
    READ_ONE_HEADER_EXTENSION_VALUE = 6
};

enum class LTP_TRAILER_RX_STATE
{
    READ_ONE_TRAILER_EXTENSION_TAG_BYTE = 0,
    READ_ONE_TRAILER_EXTENSION_LENGTH_SDNV,
    READ_ONE_TRAILER_EXTENSION_VALUE
};

enum class LTP_DATA_SEGMENT_RX_STATE
{
	READ_CLIENT_SERVICE_ID_SDNV = 0,
    READ_OFFSET_SDNV = 1,
    READ_LENGTH_SDNV = 2,
    READ_CHECKPOINT_SERIAL_NUMBER_SDNV = 3,
    READ_REPORT_SERIAL_NUMBER_SDNV = 4,
	READ_CLIENT_SERVICE_DATA = 5
};

enum class LTP_REPORT_SEGMENT_RX_STATE
{
    READ_REPORT_SERIAL_NUMBER_SDNV = 0,
    READ_CHECKPOINT_SERIAL_NUMBER_SDNV = 1,
    READ_UPPER_BOUND_SDNV = 2,
    READ_LOWER_BOUND_SDNV = 3,
    READ_RECEPTION_CLAIM_COUNT_SDNV = 4,
    READ_ONE_RECEPTION_CLAIM_OFFSET_SDNV = 5,
    READ_ONE_RECEPTION_CLAIM_LENGTH_SDNV = 6
};
enum class LTP_REPORT_ACKNOWLEDGEMENT_SEGMENT_RX_STATE
{
    READ_REPORT_SERIAL_NUMBER_SDNV = 0
};

enum class LTP_SEGMENT_TYPE_FLAGS
{
    REDDATA = 0x00,
    REDDATA_CHECKPOINT = 0x01,
    REDDATA_CHECKPOINT_ENDOFREDPART = 0x02,
    REDDATA_CHECKPOINT_ENDOFREDPART_ENDOFBLOCK = 0x03,
    GREENDATA = 0x04,
    GREENDATA_ENDOFBLOCK = 0x07,
    REPORT_SEGMENT = 0x08,
    REPORT_ACK_SEGMENT = 0x09,
    CANCEL_SEGMENT_FROM_BLOCK_SENDER = 12,
    CANCEL_ACK_SEGMENT_TO_BLOCK_SENDER = 13,
    CANCEL_SEGMENT_FROM_BLOCK_RECEIVER = 14,
    CANCEL_ACK_SEGMENT_TO_BLOCK_RECEIVER = 15,

};
enum class LTP_DATA_SEGMENT_TYPE_FLAGS //A SUBSET OF THE ABOVE FOR PARAMETER TO GENERATE DATA SEGMENTS
{
    REDDATA = 0x00,
    REDDATA_CHECKPOINT = 0x01,
    REDDATA_CHECKPOINT_ENDOFREDPART = 0x02,
    REDDATA_CHECKPOINT_ENDOFREDPART_ENDOFBLOCK = 0x03,
    GREENDATA = 0x04,
    GREENDATA_ENDOFBLOCK = 0x07
};

enum class CANCEL_SEGMENT_REASON_CODES
{
	USER_CANCELLED = 0x0, //Client service canceled session.
	UNREACHABLE = 0x1, //Unreachable client service.
	RLEXC = 0x2, //Retransmission limit exceeded.
    MISCOLORED = 0x3, //Received either a red-part data segment at block offset above any green - part data segment offset or a green - part data segment at block offset below any red - part data segment offset..
    SYSTEM_CANCELLED = 0x4, //A system error condition caused unexpected session termination.
    RXMTCYCEXC = 0x5, //Exceeded the Retransmission-Cycles limit.
	RESERVED
};



class Ltp {


public:
    struct CLASS_VISIBILITY_LTP_LIB session_id_t { //class visibility needed for LtpTimerManager
        uint64_t sessionOriginatorEngineId;
        uint64_t sessionNumber;

        LTP_LIB_EXPORT session_id_t(); //a default constructor: X()
        LTP_LIB_EXPORT session_id_t(uint64_t paramSessionOriginatorEngineId, uint64_t paramSessionNumber);
        LTP_LIB_EXPORT ~session_id_t() noexcept; //a destructor: ~X()
        LTP_LIB_EXPORT session_id_t(const session_id_t& o) noexcept; //a copy constructor: X(const X&)
        LTP_LIB_EXPORT session_id_t(session_id_t&& o) noexcept; //a move constructor: X(X&&)
        LTP_LIB_EXPORT session_id_t& operator=(const session_id_t& o) noexcept; //a copy assignment: operator=(const X&)
        LTP_LIB_EXPORT session_id_t& operator=(session_id_t&& o) noexcept; //a move assignment: operator=(X&&)
        LTP_LIB_EXPORT session_id_t& operator=(const uint64_t o) noexcept; //assign to uint64 (for template code in LtpTimerManager)
        LTP_LIB_EXPORT bool operator==(const session_id_t & o) const; //operator ==
        LTP_LIB_EXPORT bool operator==(const uint64_t o) const; //operator == (for template code in LtpTimerManager)
        LTP_LIB_EXPORT bool operator!=(const session_id_t & o) const; //operator !=
        LTP_LIB_EXPORT bool operator<(const session_id_t & o) const; //operator < so it can be used as a map key
        LTP_LIB_EXPORT friend std::ostream& operator<<(std::ostream& os, const session_id_t& o);
        LTP_LIB_EXPORT uint64_t Serialize(uint8_t * serialization) const;
    };
    struct CLASS_VISIBILITY_LTP_LIB hash_session_id_t {
        LTP_LIB_EXPORT std::size_t operator()(const session_id_t& sid) const noexcept;
    };
    struct reception_claim_t {
        uint64_t offset;
        uint64_t length;

        LTP_LIB_EXPORT reception_claim_t(); //a default constructor: X()
        LTP_LIB_EXPORT reception_claim_t(uint64_t paramOffset, uint64_t paramLength);
        LTP_LIB_EXPORT ~reception_claim_t(); //a destructor: ~X()
        LTP_LIB_EXPORT reception_claim_t(const reception_claim_t& o); //a copy constructor: X(const X&)
        LTP_LIB_EXPORT reception_claim_t(reception_claim_t&& o); //a move constructor: X(X&&)
        LTP_LIB_EXPORT reception_claim_t& operator=(const reception_claim_t& o); //a copy assignment: operator=(const X&)
        LTP_LIB_EXPORT reception_claim_t& operator=(reception_claim_t&& o); //a move assignment: operator=(X&&)
        LTP_LIB_EXPORT bool operator==(const reception_claim_t & o) const; //operator ==
        LTP_LIB_EXPORT bool operator!=(const reception_claim_t & o) const; //operator !=
        LTP_LIB_EXPORT friend std::ostream& operator<<(std::ostream& os, const reception_claim_t& o);
        LTP_LIB_EXPORT uint64_t Serialize(uint8_t * serialization) const;
    };
    struct report_segment_t {
        uint64_t reportSerialNumber;
        uint64_t checkpointSerialNumber;
        uint64_t upperBound;
        uint64_t lowerBound;
        uint64_t tmpReceptionClaimCount; //Used only by sdnv decode operations as temporary variable. Class members ignore (treat as padding bytes).
        std::vector<reception_claim_t> receptionClaims;

        LTP_LIB_EXPORT report_segment_t(); //a default constructor: X()
        LTP_LIB_EXPORT report_segment_t(uint64_t paramReportSerialNumber, uint64_t paramCheckpointSerialNumber, uint64_t paramUpperBound, uint64_t paramLowerBound, const std::vector<reception_claim_t> & paramReceptionClaims);
        LTP_LIB_EXPORT report_segment_t(uint64_t paramReportSerialNumber, uint64_t paramCheckpointSerialNumber, uint64_t paramUpperBound, uint64_t paramLowerBound, std::vector<reception_claim_t> && paramReceptionClaims);
        LTP_LIB_EXPORT ~report_segment_t(); //a destructor: ~X()
        LTP_LIB_EXPORT report_segment_t(const report_segment_t& o); //a copy constructor: X(const X&)
        LTP_LIB_EXPORT report_segment_t(report_segment_t&& o); //a move constructor: X(X&&)
        LTP_LIB_EXPORT report_segment_t& operator=(const report_segment_t& o); //a copy assignment: operator=(const X&)
        LTP_LIB_EXPORT report_segment_t& operator=(report_segment_t&& o); //a move assignment: operator=(X&&)
        LTP_LIB_EXPORT bool operator==(const report_segment_t & o) const; //operator ==
        LTP_LIB_EXPORT bool operator!=(const report_segment_t & o) const; //operator !=
        LTP_LIB_EXPORT friend std::ostream& operator<<(std::ostream& os, const report_segment_t& dt);
        LTP_LIB_EXPORT uint64_t Serialize(uint8_t * serialization) const;
        LTP_LIB_EXPORT uint64_t GetMaximumDataRequiredForSerialization() const;
    };
    struct ltp_extension_t {
        uint8_t tag;
        //uint64_t length; //shall be stored in valueVec.size()
        std::vector<uint8_t> valueVec;

        LTP_LIB_EXPORT ltp_extension_t(); //a default constructor: X()
        LTP_LIB_EXPORT ~ltp_extension_t(); //a destructor: ~X()
        LTP_LIB_EXPORT ltp_extension_t(const ltp_extension_t& o); //a copy constructor: X(const X&)
        LTP_LIB_EXPORT ltp_extension_t(ltp_extension_t&& o); //a move constructor: X(X&&)
        LTP_LIB_EXPORT ltp_extension_t& operator=(const ltp_extension_t& o); //a copy assignment: operator=(const X&)
        LTP_LIB_EXPORT ltp_extension_t& operator=(ltp_extension_t&& o); //a move assignment: operator=(X&&)
        LTP_LIB_EXPORT bool operator==(const ltp_extension_t & o) const; //operator ==
        LTP_LIB_EXPORT bool operator!=(const ltp_extension_t & o) const; //operator !=
        LTP_LIB_EXPORT void AppendSerialize(std::vector<uint8_t> & serialization) const;
        LTP_LIB_EXPORT uint64_t Serialize(uint8_t * serialization) const;
    };
    struct ltp_extensions_t {
        std::vector<ltp_extension_t> extensionsVec;

        LTP_LIB_EXPORT ltp_extensions_t(); //a default constructor: X()
        LTP_LIB_EXPORT ~ltp_extensions_t(); //a destructor: ~X()
        LTP_LIB_EXPORT ltp_extensions_t(const ltp_extensions_t& o); //a copy constructor: X(const X&)
        LTP_LIB_EXPORT ltp_extensions_t(ltp_extensions_t&& o); //a move constructor: X(X&&)
        LTP_LIB_EXPORT ltp_extensions_t& operator=(const ltp_extensions_t& o); //a copy assignment: operator=(const X&)
        LTP_LIB_EXPORT ltp_extensions_t& operator=(ltp_extensions_t&& o); //a move assignment: operator=(X&&)
        LTP_LIB_EXPORT bool operator==(const ltp_extensions_t & o) const; //operator ==
        LTP_LIB_EXPORT bool operator!=(const ltp_extensions_t & o) const; //operator !=
        LTP_LIB_EXPORT void AppendSerialize(std::vector<uint8_t> & serialization) const;
        LTP_LIB_EXPORT uint64_t Serialize(uint8_t * serialization) const;
        LTP_LIB_EXPORT uint64_t GetMaximumDataRequiredForSerialization() const;
    };
    //struct ltp_header_and_trailer_extensions_t {
    //    ltp_extensions_t headerExtensions;
    //    ltp_extensions_t trailerExtensions;
    //};
    struct data_segment_metadata_t {
        LTP_LIB_EXPORT data_segment_metadata_t();
        LTP_LIB_EXPORT data_segment_metadata_t(uint64_t paramClientServiceId, uint64_t paramOffset, uint64_t paramLength, uint64_t * paramCheckpointSerialNumber = NULL, uint64_t * paramReportSerialNumber = NULL);
        LTP_LIB_EXPORT bool operator==(const data_segment_metadata_t & o) const; //operator ==
        LTP_LIB_EXPORT bool operator!=(const data_segment_metadata_t & o) const; //operator !=
        LTP_LIB_EXPORT uint64_t Serialize(uint8_t * serialization) const;
        LTP_LIB_EXPORT uint64_t GetMaximumDataRequiredForSerialization() const;
        
        uint64_t clientServiceId;
        uint64_t offset;
        uint64_t length;
        uint64_t tmpCheckpointSerialNumber; //Used only by sdnv decode operations as temporary variable. Class members ignore (treat as padding bytes).
        uint64_t tmpReportSerialNumber; //Used only by sdnv decode operations as temporary variable. Class members ignore (treat as padding bytes).
        uint64_t * checkpointSerialNumber;
        uint64_t * reportSerialNumber;
    };
    
    
	typedef boost::function<void(uint8_t segmentTypeFlags, const session_id_t & sessionId,
        std::vector<uint8_t> & clientServiceDataVec, const data_segment_metadata_t & dataSegmentMetadata,
        Ltp::ltp_extensions_t & headerExtensions, Ltp::ltp_extensions_t & trailerExtensions)> DataSegmentContentsReadCallback_t;
    typedef boost::function<void(const session_id_t & sessionId, const report_segment_t & reportSegment,
        Ltp::ltp_extensions_t & headerExtensions, Ltp::ltp_extensions_t & trailerExtensions)> ReportSegmentContentsReadCallback_t;
    typedef boost::function<void(const session_id_t & sessionId, uint64_t reportSerialNumberBeingAcknowledged,
        Ltp::ltp_extensions_t & headerExtensions, Ltp::ltp_extensions_t & trailerExtensions)> ReportAcknowledgementSegmentContentsReadCallback_t;
    typedef boost::function<void(const session_id_t & sessionId, CANCEL_SEGMENT_REASON_CODES reasonCode, bool isFromSender,
        Ltp::ltp_extensions_t & headerExtensions, Ltp::ltp_extensions_t & trailerExtensions)> CancelSegmentContentsReadCallback_t;
    typedef boost::function<void(const session_id_t & sessionId, bool isToSender,
        Ltp::ltp_extensions_t & headerExtensions, Ltp::ltp_extensions_t & trailerExtensions)> CancelAcknowledgementSegmentContentsReadCallback_t;

    typedef boost::function<void(uint64_t sessionOriginatorEngineId)> SessionOriginatorEngineIdDecodedCallback_t;
	
    LTP_LIB_EXPORT Ltp();
    LTP_LIB_EXPORT ~Ltp();
    
    LTP_LIB_EXPORT void SetDataSegmentContentsReadCallback(const DataSegmentContentsReadCallback_t & callback);
    LTP_LIB_EXPORT void SetReportSegmentContentsReadCallback(const ReportSegmentContentsReadCallback_t & callback);
    LTP_LIB_EXPORT void SetReportAcknowledgementSegmentContentsReadCallback(const ReportAcknowledgementSegmentContentsReadCallback_t & callback);
    LTP_LIB_EXPORT void SetCancelSegmentContentsReadCallback(const CancelSegmentContentsReadCallback_t & callback);
    LTP_LIB_EXPORT void SetCancelAcknowledgementSegmentContentsReadCallback(const CancelAcknowledgementSegmentContentsReadCallback_t & callback);


    LTP_LIB_EXPORT void InitRx();
    LTP_LIB_EXPORT bool HandleReceivedChars(const uint8_t * rxVals, std::size_t numChars, std::string & errorMessage, SessionOriginatorEngineIdDecodedCallback_t * sessionOriginatorEngineIdDecodedCallbackPtr = NULL);
    LTP_LIB_EXPORT void HandleReceivedChar(const uint8_t rxVal, std::string & errorMessage);
    LTP_LIB_EXPORT bool IsAtBeginningState() const; //unit testing convenience function

    
    LTP_LIB_EXPORT static void GenerateReportAcknowledgementSegment(std::vector<uint8_t> & reportAckSegment, const session_id_t & sessionId, uint64_t reportSerialNumber);
    LTP_LIB_EXPORT static void GenerateLtpHeaderPlusDataSegmentMetadata(std::vector<uint8_t> & ltpHeaderPlusDataSegmentMetadata, LTP_DATA_SEGMENT_TYPE_FLAGS dataSegmentTypeFlags,
        const session_id_t & sessionId, const data_segment_metadata_t & dataSegmentMetadata,
        ltp_extensions_t * headerExtensions = NULL, uint8_t numTrailerExtensions = 0);
    LTP_LIB_EXPORT static void GenerateReportSegmentLtpPacket(std::vector<uint8_t> & ltpReportSegmentPacket, const session_id_t & sessionId, const report_segment_t & reportSegmentStruct,
        ltp_extensions_t * headerExtensions = NULL, ltp_extensions_t * trailerExtensions = NULL);
    LTP_LIB_EXPORT static void GenerateReportAcknowledgementSegmentLtpPacket(std::vector<uint8_t> & ltpReportAcknowledgementSegmentPacket, const session_id_t & sessionId,
        uint64_t reportSerialNumberBeingAcknowledged, ltp_extensions_t * headerExtensions = NULL, ltp_extensions_t * trailerExtensions = NULL);
    LTP_LIB_EXPORT static void GenerateCancelSegmentLtpPacket(std::vector<uint8_t> & ltpCancelSegmentPacket, const session_id_t & sessionId,
        CANCEL_SEGMENT_REASON_CODES reasonCode, bool isFromSender, ltp_extensions_t * headerExtensions = NULL, ltp_extensions_t * trailerExtensions = NULL);
    LTP_LIB_EXPORT static void GenerateCancelAcknowledgementSegmentLtpPacket(std::vector<uint8_t> & ltpCancelAcknowledgementSegmentPacket, const session_id_t & sessionId,
        bool isToSender, ltp_extensions_t * headerExtensions = NULL, ltp_extensions_t * trailerExtensions = NULL);

    LTP_LIB_EXPORT static bool GetMessageDirectionFromSegmentFlags(const uint8_t segmentFlags, bool & isSenderToReceiver);

private:
    LTP_LIB_NO_EXPORT void SetBeginningState();
    LTP_LIB_NO_EXPORT const uint8_t * NextStateAfterHeaderExtensions(const uint8_t * rxVals, std::size_t & numChars, std::string & errorMessage);
    LTP_LIB_NO_EXPORT bool NextStateAfterTrailerExtensions(std::string & errorMessage);
    LTP_LIB_NO_EXPORT const uint8_t * TryShortcutReadDataSegmentSdnvs(const uint8_t * rxVals, std::size_t & numChars, std::string & errorMessage);
    LTP_LIB_NO_EXPORT const uint8_t * TryShortcutReadReportSegmentSdnvs(const uint8_t * rxVals, std::size_t & numChars, std::string & errorMessage);
public:
	std::vector<uint8_t> m_sdnvTempVec;
    LTP_MAIN_RX_STATE m_mainRxState;
    LTP_HEADER_RX_STATE m_headerRxState;
    LTP_TRAILER_RX_STATE m_trailerRxState;
    LTP_DATA_SEGMENT_RX_STATE m_dataSegmentRxState;
    LTP_REPORT_SEGMENT_RX_STATE m_reportSegmentRxState;
    //LTP_REPORT_ACKNOWLEDGEMENT_SEGMENT_RX_STATE m_reportAcknowledgementSegmentRxState; //one state only so not needed

    uint8_t m_segmentTypeFlags;
    session_id_t m_sessionId;
    uint8_t m_numHeaderExtensionTlvs;
    uint8_t m_numTrailerExtensionTlvs;
    ltp_extensions_t m_headerExtensions;
    ltp_extensions_t m_trailerExtensions;
    uint64_t m_currentHeaderExtensionLength;
    uint64_t m_currentTrailerExtensionLength;

    data_segment_metadata_t m_dataSegmentMetadata;
    std::vector<uint8_t> m_dataSegment_clientServiceData;
    
    report_segment_t m_reportSegment;

    uint64_t m_reportAcknowledgementSegment_reportSerialNumber;

    uint8_t m_cancelSegment_reasonCode;
        
	//callback functions
	DataSegmentContentsReadCallback_t m_dataSegmentContentsReadCallback;
    ReportSegmentContentsReadCallback_t m_reportSegmentContentsReadCallback;
    ReportAcknowledgementSegmentContentsReadCallback_t m_reportAcknowledgementSegmentContentsReadCallback;
    CancelSegmentContentsReadCallback_t m_cancelSegmentContentsReadCallback;
    CancelAcknowledgementSegmentContentsReadCallback_t m_cancelAcknowledgementSegmentContentsReadCallback;
};

#endif // LTP_H

