/**
 * @file TestUdpBatchSender.cpp
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
 */

#include <boost/test/unit_test.hpp>
#include "UdpBatchSender.h"
#include <iostream>
#include <boost/thread.hpp>

static std::vector<uint8_t> g_udpReceiveBuffer;
static std::vector<std::vector<uint8_t> > g_udpPacketsReceived;
static boost::asio::ip::udp::endpoint g_remoteEndpoint;
static void HandleUdpReceive(const boost::system::error_code& error, std::size_t bytesTransferred);
static boost::asio::ip::udp::socket* g_udpSocketPtr;
static boost::asio::deadline_timer* g_deadlineTimerPtr;
static volatile uint64_t g_constBufferVecsCallbackSize;
static volatile uint64_t g_underlyingDataToDeleteOnSentCallbackSize;
static volatile uint64_t g_underlyingCsDataToDeleteOnSentCallbackSize;
static volatile bool g_sentCallbackWasSuccessful;
static boost::condition_variable g_conditionVariableSentPackets;
static boost::mutex g_conditionVariableSentPacketsMutex;

static void OnSentPacketsCallback(bool success, std::vector<std::vector<boost::asio::const_buffer> >& constBufferVecs,
    std::vector<std::shared_ptr<std::vector<std::vector<uint8_t> > > >& underlyingDataToDeleteOnSentCallbackVec,
    std::vector<std::shared_ptr<LtpClientServiceDataToSend> >& underlyingCsDataToDeleteOnSentCallbackVec)
{
    g_constBufferVecsCallbackSize = constBufferVecs.size();
    g_underlyingDataToDeleteOnSentCallbackSize = underlyingDataToDeleteOnSentCallbackVec.size();
    g_underlyingCsDataToDeleteOnSentCallbackSize = underlyingCsDataToDeleteOnSentCallbackVec.size();
    g_conditionVariableSentPacketsMutex.lock();
    g_sentCallbackWasSuccessful = success; //must be last assignment as this is the "done" flag
    g_conditionVariableSentPacketsMutex.unlock();
    g_conditionVariableSentPackets.notify_one();
}

static void StartUdpReceive() {
    g_udpReceiveBuffer.resize(100);
    g_udpSocketPtr->async_receive_from(
        boost::asio::buffer(g_udpReceiveBuffer),
        g_remoteEndpoint,
        boost::bind(&HandleUdpReceive,
            boost::asio::placeholders::error,
            boost::asio::placeholders::bytes_transferred));
}

static void HandleUdpReceive(const boost::system::error_code& error, std::size_t bytesTransferred) {
    if (!error) {
        g_udpReceiveBuffer.resize(bytesTransferred);
        g_udpPacketsReceived.push_back(std::move(g_udpReceiveBuffer));
        if (g_udpPacketsReceived.size() < 3) {
            StartUdpReceive(); //restart operation only if there was no error
        }
        else { //received all
            g_deadlineTimerPtr->cancel();
        }
    }
    else if (error != boost::asio::error::operation_aborted) {
        g_deadlineTimerPtr->cancel();
        std::cout << "unknown error in UdpBatchSenderTestCase HandleUdpReceive: " << error.message() << "\n";
        BOOST_ERROR("");
    }
}

static void DurationEndedThreadFunction(const boost::system::error_code& e) {
    if (e != boost::asio::error::operation_aborted) {
        // Timer was not cancelled, take necessary action.
        g_udpSocketPtr->cancel();
        BOOST_ERROR("UdpBatchSenderTestCase failed due to no packets received after 5 seconds");
    }
    else if (e == boost::asio::error::operation_aborted) {
        // Timer cancelled (success)
    }
    else {
        g_udpSocketPtr->cancel();
        BOOST_ERROR("Unknown error occurred in DurationEndedThreadFunction");
    }
}

BOOST_AUTO_TEST_CASE(UdpBatchSenderTestCase)
{
    //test UdpBatchSender
    {
        //first set up a receiver
        boost::asio::io_service ioService;
        boost::asio::ip::udp::socket udpSocket(ioService); //receiving only
        boost::asio::deadline_timer deadlineTimer(ioService);
        g_udpSocketPtr = &udpSocket;
        g_deadlineTimerPtr = &deadlineTimer;

        try {
            udpSocket.open(boost::asio::ip::udp::v4());
            udpSocket.bind(boost::asio::ip::udp::endpoint(boost::asio::ip::udp::v4(), 1113));
        }
        catch (const boost::system::system_error& e) {
            std::cerr << "Error: " << e.what() << std::endl;
            BOOST_ERROR("Could not bind on UDP port 1113 in UdpBatchSenderTestCase");
        }

        UdpBatchSender ubs;
        ubs.SetOnSentPacketsCallback(boost::bind(&OnSentPacketsCallback, boost::placeholders::_1, boost::placeholders::_2, boost::placeholders::_3, boost::placeholders::_4));
        BOOST_REQUIRE(ubs.Init("localhost", 1112)); //intentionally set the wrong port, correct it on the next line
        ubs.SetEndpointAndReconnect_ThreadSafe("localhost", 1113);
        unsigned int successfulTests = 0;
        
        
        for (unsigned int count = 0; count < 10; ++count) {
            g_udpPacketsReceived.clear();
            std::vector<std::vector<boost::asio::const_buffer> > constBufferVecs;
            constBufferVecs.resize(3);

            //send packet with "one"
            constBufferVecs[0].resize(1);
            constBufferVecs[0][0] = boost::asio::buffer("one", 3);

            //send packet with "twothree"
            constBufferVecs[1].resize(2);
            constBufferVecs[1][0] = boost::asio::buffer("two", 3);
            constBufferVecs[1][1] = boost::asio::buffer("three", 5);

            //send packet with "fourfivesix"
            constBufferVecs[2].resize(3);
            constBufferVecs[2][0] = boost::asio::buffer("four", 4);
            constBufferVecs[2][1] = boost::asio::buffer("five", 4);
            constBufferVecs[2][2] = boost::asio::buffer("six", 3);


            std::vector<std::shared_ptr<std::vector<std::vector<uint8_t> > > > underlyingDataToDeleteOnSentCallbackVec; //null not needed
            underlyingDataToDeleteOnSentCallbackVec.resize(10); //for testing
            std::vector<std::shared_ptr<LtpClientServiceDataToSend> > underlyingCsDataToDeleteOnSentCallbackVec; //null not needed
            underlyingCsDataToDeleteOnSentCallbackVec.resize(5); //for testing

            BOOST_REQUIRE_EQUAL(constBufferVecs.size(), 3);
            BOOST_REQUIRE_EQUAL(underlyingDataToDeleteOnSentCallbackVec.size(), 10);
            g_constBufferVecsCallbackSize = 0; //modified after callback
            g_underlyingDataToDeleteOnSentCallbackSize = 0; //modified after callback
            g_underlyingCsDataToDeleteOnSentCallbackSize = 0; //modified after callback
            g_sentCallbackWasSuccessful = false; //modified after callback

            deadlineTimer.expires_from_now(boost::posix_time::seconds(5)); //fail after 5 seconds
            deadlineTimer.async_wait(boost::bind(&DurationEndedThreadFunction, boost::asio::placeholders::error));
            StartUdpReceive();

            //std::cout << "starting UdpBatchSenderTestCase send/receive operation\n";
            ubs.QueueSendPacketsOperation_ThreadSafe(constBufferVecs, underlyingDataToDeleteOnSentCallbackVec, underlyingCsDataToDeleteOnSentCallbackVec); //data gets stolen
            ioService.run();
            ioService.reset();

            //wait for OnSentPacketsCallback, because it's possible for receiver callback to be called first
            {
                boost::mutex::scoped_lock cvLock(g_conditionVariableSentPacketsMutex); //must lock before checking the flag
                while (!g_sentCallbackWasSuccessful) {
                    g_conditionVariableSentPackets.wait(cvLock);
                }
            }

            BOOST_REQUIRE_EQUAL(constBufferVecs.size(), 0); //stolen and empty
            BOOST_REQUIRE_EQUAL(underlyingDataToDeleteOnSentCallbackVec.size(), 0); //stolen and empty
            BOOST_REQUIRE_EQUAL(underlyingCsDataToDeleteOnSentCallbackVec.size(), 0); //stolen and empty
            BOOST_REQUIRE_EQUAL(g_udpPacketsReceived.size(), 3);

            BOOST_REQUIRE_EQUAL(g_udpPacketsReceived[0].size(), 3);
            BOOST_REQUIRE_EQUAL(g_udpPacketsReceived[1].size(), 8);
            BOOST_REQUIRE_EQUAL(g_udpPacketsReceived[2].size(), 11);

            const std::string p0((const char*)(g_udpPacketsReceived[0].data()), (const char*)(g_udpPacketsReceived[0].data() + g_udpPacketsReceived[0].size()));
            const std::string p1((const char*)(g_udpPacketsReceived[1].data()), (const char*)(g_udpPacketsReceived[1].data() + g_udpPacketsReceived[1].size()));
            const std::string p2((const char*)(g_udpPacketsReceived[2].data()), (const char*)(g_udpPacketsReceived[2].data() + g_udpPacketsReceived[2].size()));

            BOOST_REQUIRE_EQUAL(p0, "one");
            BOOST_REQUIRE_EQUAL(p1, "twothree");
            BOOST_REQUIRE_EQUAL(p2, "fourfivesix");

            BOOST_REQUIRE_EQUAL(g_constBufferVecsCallbackSize, 3);
            BOOST_REQUIRE_EQUAL(g_underlyingDataToDeleteOnSentCallbackSize, 10);
            BOOST_REQUIRE_EQUAL(g_underlyingCsDataToDeleteOnSentCallbackSize, 5);
            BOOST_REQUIRE(g_sentCallbackWasSuccessful);

            ++successfulTests;
        }
        BOOST_REQUIRE_EQUAL(successfulTests, 10);
    }
}
