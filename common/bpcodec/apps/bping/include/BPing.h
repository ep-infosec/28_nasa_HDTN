#ifndef _BPING_H
#define _BPING_H 1

#include "app_patterns/BpSourcePattern.h"

class BPing : public BpSourcePattern {
public:
    BPing();
    virtual ~BPing();
    
protected:
    virtual uint64_t GetNextPayloadLength_Step1();
    virtual bool CopyPayload_Step2(uint8_t * destinationBuffer);
    virtual bool ProcessNonAdminRecordBundlePayload(const uint8_t * data, const uint64_t size);
private:
    uint64_t m_bpingSequenceNumber;
};


#endif //_BPING_H
