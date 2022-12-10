#include "Outduct.h"
#include <iostream>
#include <boost/make_unique.hpp>
#include <memory>


Outduct::Outduct(const outduct_element_config_t & outductConfig, const uint64_t outductUuid) :
    m_outductConfig(outductConfig),
    m_outductUuid(outductUuid)
{}
Outduct::~Outduct() {}

uint64_t Outduct::GetOutductUuid() const {
    return m_outductUuid;
}
uint64_t Outduct::GetOutductMaxNumberOfBundlesInPipeline() const {
    return m_outductConfig.maxNumberOfBundlesInPipeline;
}
uint64_t Outduct::GetOutductMaxSumOfBundleBytesInPipeline() const {
    return m_outductConfig.maxSumOfBundleBytesInPipeline;
}
uint64_t Outduct::GetOutductNextHopNodeId() const {
    return m_outductConfig.nextHopNodeId;
}
std::string Outduct::GetConvergenceLayerName() const {
    return m_outductConfig.convergenceLayer;
}
uint64_t Outduct::GetOutductTelemetry(uint8_t* data, uint64_t bufferSize) {
    return 0;
}

void Outduct::SetOnFailedBundleVecSendCallback(const OnFailedBundleVecSendCallback_t& callback) {}
void Outduct::SetOnFailedBundleZmqSendCallback(const OnFailedBundleZmqSendCallback_t& callback) {}
void Outduct::SetOnSuccessfulBundleSendCallback(const OnSuccessfulBundleSendCallback_t& callback) {}
void Outduct::SetOnOutductLinkStatusChangedCallback(const OnOutductLinkStatusChangedCallback_t& callback) {}
void Outduct::SetUserAssignedUuid(uint64_t userAssignedUuid) {}
