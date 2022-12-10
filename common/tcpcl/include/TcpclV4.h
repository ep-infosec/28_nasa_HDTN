/**
 * @file TcpclV4.h
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
 * This TcpclV4 class implements RFC 9174 (version 4 of the TCP Convergence-Layer Protocol).
 * It defines data structures used in the TCPCL Version 4 library,
 * defines methods for encoding TCPCL Version 4 headers (segments),
 * and defines methods for finite-state machine (FSM) receiving
 * of all bytes or partial bytes of a TCPCL Version 4 message with custom callback functions
 * that (if the function is defined) get called whenever the appropriate number of bytes is received.
 */

#ifndef TCPCLV4_H
#define TCPCLV4_H 1

#include <string>
#include <memory>
#include <boost/integer.hpp>
#include <boost/function.hpp>
#include <vector>
#include "PaddedVectorUint8.h"
#include "tcpcl_lib_export.h"

enum class TCPCLV4_MAIN_RX_STATE
{
    READ_CONTACT_HEADER = 0,
    READ_MESSAGE_TYPE_BYTE,
    READ_DATA_SEGMENT,
    READ_ACK_SEGMENT,
    READ_TRANSFER_REFUSAL,
    READ_MESSAGE_REJECTION,
    READ_LENGTH_SEGMENT,
    READ_SESSION_TERMINATION,
    READ_SESSION_INIT
};

enum class TCPCLV4_CONTACT_HEADER_RX_STATE
{
    READ_SYNC_1 = 0,
    READ_SYNC_2,
    READ_SYNC_3,
    READ_SYNC_4,
    READ_VERSION,
    READ_FLAGS,
    READ_KEEPALIVE_INTERVAL_BYTE1,
    READ_KEEPALIVE_INTERVAL_BYTE2,
    READ_LOCAL_EID_LENGTH_SDNV,
    READ_LOCAL_EID_STRING
};

enum class TCPCLV4_DATA_SEGMENT_RX_STATE
{
    READ_MESSAGE_FLAGS_BYTE = 0,
    READ_TRANSFER_ID_U64,
    READ_START_SEGMENT_TRANSFER_EXTENSION_ITEMS_LENGTH_U32,
    READ_ONE_START_SEGMENT_TRANSFER_EXTENSION_ITEM_FLAG,
    READ_ONE_START_SEGMENT_TRANSFER_EXTENSION_ITEM_TYPE,
    READ_ONE_START_SEGMENT_TRANSFER_EXTENSION_ITEM_LENGTH,
    READ_ONE_START_SEGMENT_TRANSFER_EXTENSION_ITEM_VALUE,
    READ_DATA_LENGTH_U64,
    READ_DATA_CONTENTS
};

enum class TCPCLV4_DATA_ACK_RX_STATE
{
    READ_MESSAGE_FLAGS_BYTE = 0,
    READ_TRANSFER_ID_U64,
    READ_ACKNOWLEDGED_LENGTH_U64
};

enum class TCPCLV4_MESSAGE_REJECT_RX_STATE
{
    READ_REASON_CODE_BYTE = 0,
    READ_REJECTED_MESSAGE_HEADER
};

enum class TCPCLV4_TRANSFER_REFUSAL_RX_STATE
{
    READ_REASON_CODE_BYTE = 0,
    READ_TRANSFER_ID
};

enum class TCPCLV4_SESSION_TERMINATION_RX_STATE
{
    READ_MESSAGE_FLAGS_BYTE = 0,
    READ_REASON_CODE_BYTE
};

enum class TCPCLV4_SESSION_INIT_RX_STATE
{
    READ_KEEPALIVE_INTERVAL_U16 = 0,
    READ_SEGMENT_MRU_U64,
    READ_TRANSFER_MRU_U64,
    READ_NODE_ID_LENGTH_U16,
    READ_NODE_ID,
    READ_SESSION_EXTENSION_ITEMS_LENGTH_U32,
    READ_ONE_SESSION_EXTENSION_ITEM_FLAG,
    READ_ONE_SESSION_EXTENSION_ITEM_TYPE,
    READ_ONE_SESSION_EXTENSION_ITEM_LENGTH,
    READ_ONE_SESSION_EXTENSION_ITEM_VALUE,
};

enum class TCPCLV4_MESSAGE_TYPE_BYTE_CODES
{
    RESERVED = 0x0,
    XFER_SEGMENT = 0x1,
    XFER_ACK = 0x2,
    XFER_REFUSE = 0x3,
    KEEPALIVE = 0x4,
    SESS_TERM = 0x5,
    MSG_REJECT = 0x6,
    SESS_INIT = 0x7
};

enum class TCPCLV4_SESSION_TERMINATION_REASON_CODES
{
    UNKNOWN = 0x0, //A termination reason is not available.
    IDLE_TIMEOUT = 0x1, //The session is being terminated due to idleness.
    VERSION_MISMATCH = 0x2, //The entity cannot conform to the specified TCPCL protocol version.
    BUSY = 0x3, //The entity is too busy to handle the current session.
    CONTACT_FAILURE = 0x4, //The entity cannot interpret or negotiate a Contact Header or SESS_INIT option.
    RESOURCE_EXHAUSTION = 0x5 //The entity has run into some resource limit and cannot continue the session.
};

enum class TCPCLV4_MESSAGE_REJECT_REASON_CODES
{
    MESSAGE_TYPE_UNKNOWN = 0x1, //A message was received with a Message Type code unknown to the TCPCL entity.
    MESSAGE_UNSUPPORTED = 0x2, //A message was received but the TCPCL entity cannot comply with the message contents.
    MESSAGE_UNEXPECTED = 0x3 //A message was received while the session is in a state in which the message is not expected.
};

enum class TCPCLV4_TRANSFER_REFUSE_REASON_CODES
{
    REFUSAL_REASON_UNKNOWN = 0x0, //Reason for refusal is unknown or not specified. //REASON_UNKNOW taken in windows.h
    REFUSAL_REASON_ALREADY_COMPLETED = 0x1, //The receiver already has the complete bundle.The sender MAY consider the bundle as completely received.
    REFUSAL_REASON_NO_RESOURCES = 0x2, //The receiver's resources are exhausted. The sender SHOULD apply reactive bundle fragmentation before retrying.
    REFUSAL_REASON_RETRANSMIT = 0x3, //The receiver has encountered a problem that requires the bundle to be retransmitted in its entirety.
    REFUSAL_REASON_NOT_ACCEPTABLE = 0x4, //Some issue with the bundle data or the transfer extension data was encountered. The sender SHOULD NOT retry the same bundle with the same extensions.
    REFUSAL_REASON_EXTENSION_FAILURE = 0x5, //A failure processing the Transfer Extension Items has occurred.
    REFUSAL_REASON_SESSION_TERMINATING = 0x6 //The receiving entity is in the process of terminating the session. The sender MAY retry the same bundle at a later time in a different session.
};


class TcpclV4 {
public:
    struct tcpclv4_extension_t {
        uint8_t flags;
        uint16_t type;
        //uint16_t length; //shall be stored in valueVec.size()
        std::vector<uint8_t> valueVec;

        TCPCL_LIB_EXPORT tcpclv4_extension_t(); //a default constructor: X()
        TCPCL_LIB_EXPORT tcpclv4_extension_t(bool isCriticalFlag, uint16_t itemType, const std::vector<uint8_t> & valueAsVec);
        TCPCL_LIB_EXPORT tcpclv4_extension_t(bool isCriticalFlag, uint16_t itemType, std::vector<uint8_t> && valueAsVec);
        TCPCL_LIB_EXPORT ~tcpclv4_extension_t(); //a destructor: ~X()
        TCPCL_LIB_EXPORT tcpclv4_extension_t(const tcpclv4_extension_t& o); //a copy constructor: X(const X&)
        TCPCL_LIB_EXPORT tcpclv4_extension_t(tcpclv4_extension_t&& o); //a move constructor: X(X&&)
        TCPCL_LIB_EXPORT tcpclv4_extension_t& operator=(const tcpclv4_extension_t& o); //a copy assignment: operator=(const X&)
        TCPCL_LIB_EXPORT tcpclv4_extension_t& operator=(tcpclv4_extension_t&& o); //a move assignment: operator=(X&&)
        TCPCL_LIB_EXPORT bool operator==(const tcpclv4_extension_t & o) const; //operator ==
        TCPCL_LIB_EXPORT bool operator!=(const tcpclv4_extension_t & o) const; //operator !=
        TCPCL_LIB_EXPORT bool IsCritical() const;
        TCPCL_LIB_EXPORT void AppendSerialize(std::vector<uint8_t> & serialization) const;
        TCPCL_LIB_EXPORT uint64_t Serialize(uint8_t * serialization) const;

        //helpers
        TCPCL_LIB_EXPORT static uint64_t SerializeTransferLengthExtension(uint8_t * serialization, const uint64_t totalLength);
        static constexpr uint64_t SIZE_OF_SERIALIZED_TRANSFER_LENGTH_EXTENSION = 5 + sizeof(uint64_t); //5 bytes of flags, type, length
    };
    struct tcpclv4_extensions_t {
        std::vector<tcpclv4_extension_t> extensionsVec;

        TCPCL_LIB_EXPORT tcpclv4_extensions_t(); //a default constructor: X()
        TCPCL_LIB_EXPORT ~tcpclv4_extensions_t(); //a destructor: ~X()
        TCPCL_LIB_EXPORT tcpclv4_extensions_t(const tcpclv4_extensions_t& o); //a copy constructor: X(const X&)
        TCPCL_LIB_EXPORT tcpclv4_extensions_t(tcpclv4_extensions_t&& o); //a move constructor: X(X&&)
        TCPCL_LIB_EXPORT tcpclv4_extensions_t& operator=(const tcpclv4_extensions_t& o); //a copy assignment: operator=(const X&)
        TCPCL_LIB_EXPORT tcpclv4_extensions_t& operator=(tcpclv4_extensions_t&& o); //a move assignment: operator=(X&&)
        TCPCL_LIB_EXPORT bool operator==(const tcpclv4_extensions_t & o) const; //operator ==
        TCPCL_LIB_EXPORT bool operator!=(const tcpclv4_extensions_t & o) const; //operator !=
        TCPCL_LIB_EXPORT void AppendSerialize(std::vector<uint8_t> & serialization) const;
        TCPCL_LIB_EXPORT uint64_t Serialize(uint8_t * serialization) const;
        TCPCL_LIB_EXPORT uint64_t GetTotalDataRequiredForSerialization() const;
    };
    struct tcpclv4_ack_t {
        bool isStartSegment;
        bool isEndSegment;
        uint64_t transferId;
        uint64_t totalBytesAcknowledged;

        TCPCL_LIB_EXPORT tcpclv4_ack_t(); //a default constructor: X()
        TCPCL_LIB_EXPORT tcpclv4_ack_t(bool paramIsStartSegment, bool paramIsEndSegment, uint64_t paramTransferId, uint64_t paramTotalBytesAcknowledged);
        TCPCL_LIB_EXPORT ~tcpclv4_ack_t(); //a destructor: ~X()
        TCPCL_LIB_EXPORT tcpclv4_ack_t(const tcpclv4_ack_t& o); //a copy constructor: X(const X&)
        TCPCL_LIB_EXPORT tcpclv4_ack_t(tcpclv4_ack_t&& o); //a move constructor: X(X&&)
        TCPCL_LIB_EXPORT tcpclv4_ack_t& operator=(const tcpclv4_ack_t& o); //a copy assignment: operator=(const X&)
        TCPCL_LIB_EXPORT tcpclv4_ack_t& operator=(tcpclv4_ack_t&& o); //a move assignment: operator=(X&&)
        TCPCL_LIB_EXPORT bool operator==(const tcpclv4_ack_t & o) const; //operator ==
        TCPCL_LIB_EXPORT bool operator!=(const tcpclv4_ack_t & o) const; //operator !=
        TCPCL_LIB_EXPORT friend std::ostream& operator<<(std::ostream& os, const tcpclv4_ack_t& o);
    };

public:
    typedef boost::function<void(padded_vector_uint8_t & dataSegmentDataVec, bool isStartFlag, bool isEndFlag,
        uint64_t transferId, const tcpclv4_extensions_t & transferExtensions)> DataSegmentContentsReadCallback_t;
    typedef boost::function<void(bool remoteHasEnabledTlsSecurity)> ContactHeaderReadCallback_t;
    typedef boost::function<void(uint16_t keepAliveIntervalSeconds, uint64_t segmentMru, uint64_t transferMru,
        const std::string & remoteNodeEidUri, const tcpclv4_extensions_t & sessionExtensions)> SessionInitCallback_t;
    typedef boost::function<void(const tcpclv4_ack_t & ack)> AckSegmentReadCallback_t;
    typedef boost::function<void(TCPCLV4_MESSAGE_REJECT_REASON_CODES refusalCode, uint8_t rejectedMessageHeader)> MessageRejectCallback_t;
    typedef boost::function<void(TCPCLV4_TRANSFER_REFUSE_REASON_CODES refusalCode, uint64_t transferId)> BundleRefusalCallback_t;
    typedef boost::function<void()> KeepAliveCallback_t;
    typedef boost::function<void(TCPCLV4_SESSION_TERMINATION_REASON_CODES terminationReasonCode, bool isAckOfAnEarlierSessionTerminationMessage)> SessionTerminationMessageCallback_t;

    TCPCL_LIB_EXPORT TcpclV4();
    TCPCL_LIB_EXPORT ~TcpclV4();
    TCPCL_LIB_EXPORT void SetDataSegmentContentsReadCallback(const DataSegmentContentsReadCallback_t & callback);
    TCPCL_LIB_EXPORT void SetContactHeaderReadCallback(const ContactHeaderReadCallback_t & callback);
    TCPCL_LIB_EXPORT void SetSessionInitReadCallback(const SessionInitCallback_t & callback);
    TCPCL_LIB_EXPORT void SetAckSegmentReadCallback(const AckSegmentReadCallback_t & callback);
    TCPCL_LIB_EXPORT void SetBundleRefusalCallback(const BundleRefusalCallback_t & callback);
    TCPCL_LIB_EXPORT void SetMessageRejectCallback(const MessageRejectCallback_t & callback);
    TCPCL_LIB_EXPORT void SetKeepAliveCallback(const KeepAliveCallback_t & callback);
    TCPCL_LIB_EXPORT void SetSessionTerminationMessageCallback(const SessionTerminationMessageCallback_t & callback);
    TCPCL_LIB_EXPORT void SetMaxReceiveBundleSizeBytes(const uint64_t maxRxBundleSizeBytes);

    TCPCL_LIB_EXPORT void InitRx();
    TCPCL_LIB_EXPORT void HandleReceivedChars(const uint8_t * rxVals, std::size_t numChars);
    TCPCL_LIB_EXPORT void HandleReceivedChar(const uint8_t rxVal);
    TCPCL_LIB_EXPORT static void GenerateContactHeader(std::vector<uint8_t> & hdr, bool remoteHasEnabledTlsSecurity);
    TCPCL_LIB_EXPORT static bool GenerateSessionInitMessage(std::vector<uint8_t> & msg, uint16_t keepAliveIntervalSeconds, uint64_t segmentMru, uint64_t transferMru,
        const std::string & myNodeEidUri, const tcpclv4_extensions_t & sessionExtensions);

    //data segment with payload
    TCPCL_LIB_EXPORT static bool GenerateNonFragmentedDataSegment(std::vector<uint8_t> & dataSegment, uint64_t transferId,
        const uint8_t * contents, uint64_t sizeContents);
    TCPCL_LIB_EXPORT static bool GenerateNonFragmentedDataSegment(std::vector<uint8_t> & dataSegment, uint64_t transferId,
        const uint8_t * contents, uint64_t sizeContents, const tcpclv4_extensions_t & transferExtensions);
    TCPCL_LIB_EXPORT static bool GenerateStartDataSegment(std::vector<uint8_t> & dataSegment, bool isEndSegment, uint64_t transferId,
        const uint8_t * contents, uint64_t sizeContents, const tcpclv4_extensions_t & transferExtensions);
    TCPCL_LIB_EXPORT static bool GenerateFragmentedStartDataSegmentWithLengthExtension(std::vector<uint8_t> & dataSegment, uint64_t transferId,
        const uint8_t * contents, uint64_t sizeContents, uint64_t totalBundleLengthToBeSent);
    TCPCL_LIB_EXPORT static bool GenerateNonStartDataSegment(std::vector<uint8_t> & dataSegment, bool isEndSegment, uint64_t transferId,
        const uint8_t * contents, uint64_t sizeContents);

    //data segment header only
    TCPCL_LIB_EXPORT static bool GenerateNonFragmentedDataSegmentHeaderOnly(std::vector<uint8_t> & dataSegmentHeaderDataVec, uint64_t transferId,
        uint64_t sizeContents);
    TCPCL_LIB_EXPORT static bool GenerateNonFragmentedDataSegmentHeaderOnly(std::vector<uint8_t> & dataSegmentHeaderDataVec, uint64_t transferId,
        uint64_t sizeContents, const tcpclv4_extensions_t & transferExtensions);
    TCPCL_LIB_EXPORT static bool GenerateStartDataSegmentHeaderOnly(std::vector<uint8_t> & dataSegmentHeaderDataVec, bool isEndSegment, uint64_t transferId,
        uint64_t sizeContents, const tcpclv4_extensions_t & transferExtensions);
    TCPCL_LIB_EXPORT static bool GenerateFragmentedStartDataSegmentWithLengthExtensionHeaderOnly(std::vector<uint8_t> & dataSegmentHeaderDataVec, uint64_t transferId,
        uint64_t sizeContents, uint64_t totalBundleLengthToBeSent);
    TCPCL_LIB_EXPORT static bool GenerateNonStartDataSegmentHeaderOnly(std::vector<uint8_t> & dataSegmentHeaderDataVec, bool isEndSegment, uint64_t transferId,
        uint64_t sizeContents);

    TCPCL_LIB_EXPORT static bool GenerateAckSegment(std::vector<uint8_t> & ackSegment, const tcpclv4_ack_t & ack);
    TCPCL_LIB_EXPORT static bool GenerateAckSegment(std::vector<uint8_t> & ackSegment, bool isStartSegment, bool isEndSegment, uint64_t transferId, uint64_t totalBytesAcknowledged);
    TCPCL_LIB_EXPORT static void GenerateBundleRefusal(std::vector<uint8_t> & refusalMessage, TCPCLV4_TRANSFER_REFUSE_REASON_CODES refusalCode, uint64_t transferId);
    TCPCL_LIB_EXPORT static void GenerateMessageRejection(std::vector<uint8_t> & rejectionMessage, TCPCLV4_MESSAGE_REJECT_REASON_CODES rejectionCode, uint8_t rejectedMessageHeader);
    TCPCL_LIB_EXPORT static void GenerateKeepAliveMessage(std::vector<uint8_t> & keepAliveMessage);
    TCPCL_LIB_EXPORT static void GenerateSessionTerminationMessage(std::vector<uint8_t> & sessionTerminationMessage,
        TCPCLV4_SESSION_TERMINATION_REASON_CODES sessionTerminationReasonCode, bool isAckOfAnEarlierSessionTerminationMessage);
public:
    uint64_t M_MAX_RX_BUNDLE_SIZE_BYTES;
    TCPCLV4_MAIN_RX_STATE m_mainRxState;
    TCPCLV4_CONTACT_HEADER_RX_STATE m_contactHeaderRxState;
    TCPCLV4_DATA_SEGMENT_RX_STATE m_dataSegmentRxState;
    TCPCLV4_DATA_ACK_RX_STATE m_dataAckRxState;
    TCPCLV4_MESSAGE_REJECT_RX_STATE m_messageRejectRxState;
    TCPCLV4_TRANSFER_REFUSAL_RX_STATE m_transferRefusalRxState;
    TCPCLV4_SESSION_TERMINATION_RX_STATE m_sessionTerminationRxState;
    TCPCLV4_SESSION_INIT_RX_STATE m_sessionInitRxState;

    //contact header
    bool m_remoteHasEnabledTlsSecurity;

    
    TCPCLV4_MESSAGE_TYPE_BYTE_CODES m_messageTypeByte;

    //session init
    uint16_t m_keepAliveInterval;
    uint64_t m_segmentMru;
    uint64_t m_transferMru;
    uint16_t m_remoteNodeUriLength;
    std::string m_remoteNodeUriStr;
    uint32_t m_sessionExtensionItemsLengthBytes;
    uint32_t m_currentCountOfSessionExtensionEncodedBytes;
    tcpclv4_extensions_t m_sessionExtensions;
    uint16_t m_currentSessionExtensionLength;

    //misc
    uint8_t m_readValueByteIndex;
    

    //data segment
    uint8_t m_messageFlags;
    bool m_dataSegmentStartFlag;
    bool m_dataSegmentEndFlag;
    uint64_t m_transferId;
    uint32_t m_transferExtensionItemsLengthBytes;
    uint32_t m_currentCountOfTransferExtensionEncodedBytes;
    tcpclv4_extensions_t m_transferExtensions;
    uint16_t m_currentTransferExtensionLength;
    uint64_t m_dataSegmentLength;
    padded_vector_uint8_t m_dataSegmentDataVec;

    //ack segment
    uint8_t m_ackFlags;
    tcpclv4_ack_t m_ack;

    //refuse message
    uint8_t m_messageRejectionReasonCode;
    uint8_t m_rejectedMessageHeader;

    //refuse bundle transfer
    uint8_t m_bundleTranferRefusalReasonCode;
    uint64_t m_bundleTranferRefusalTransferId;

    //next bundle length
    uint64_t m_nextBundleLength;

    //session termination
    uint8_t m_sessionTerminationFlags;
    bool m_isSessionTerminationAck;
    //uint64_t m_shutdownReconnectionDelay;
    TCPCLV4_SESSION_TERMINATION_REASON_CODES m_sessionTerminationReasonCode;

    //callback functions
    ContactHeaderReadCallback_t m_contactHeaderReadCallback;
    SessionInitCallback_t m_sessionInitCallback;
    DataSegmentContentsReadCallback_t m_dataSegmentContentsReadCallback;
    AckSegmentReadCallback_t m_ackSegmentReadCallback;
    MessageRejectCallback_t m_messageRejectCallback;
    BundleRefusalCallback_t m_bundleRefusalCallback;
    KeepAliveCallback_t m_keepAliveCallback;
    SessionTerminationMessageCallback_t m_sessionTerminationMessageCallback;
};

#endif // TCPCLV4_H

