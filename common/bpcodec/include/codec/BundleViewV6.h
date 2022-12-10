#ifndef BUNDLE_VIEW_V6_H
#define BUNDLE_VIEW_V6_H 1

#include "codec/bpv6.h"
#include <cstdint>
#include <vector>
#include <list>
#include <map>
#include <boost/asio/buffer.hpp>

/*

Each bundle shall be a concatenated sequence of at least two block
structures.  The first block in the sequence must be a primary bundle
block, and no bundle may have more than one primary bundle block.
Additional bundle protocol blocks of other types may follow the
primary block to support extensions to the bundle protocol, such as
the Bundle Security Protocol [BSP].  At most one of the blocks in the
sequence may be a payload block.  The last block in the sequence must
have the "last block" flag (in its block processing control flags)
set to 1; for every other block in the bundle after the primary
block, this flag must be set to zero.

To keep from possibly invalidating bundle security, the sequencing
of the blocks in a forwarded bundle must not be changed as it
transits a node; received blocks must be transmitted in the same
relative order as that in which they were received.  While blocks
may be added to bundles as they transit intermediate nodes,
removal of blocks that do not have their 'Discard block if it
can't be processed' flag in the block processing control flags set
to 1 may cause security to fail.

Bundle security must not be invalidated by forwarding nodes even
though they themselves might not use the Bundle Security Protocol.
In particular, the sequencing of the blocks in a forwarded bundle
must not be changed as it transits a node; received blocks must be
transmitted in the same relative order as that in which they were
received.  While blocks may be added to bundles as they transit
intermediate nodes, removal of blocks that do not have their 'Discard
block if it can't be processed' flag in the block processing control
flags set to 1 may cause security to fail.

A bundle MAY have multiple security blocks.
*/

class BundleViewV6 {

public:
    struct Bpv6PrimaryBlockView {
        Bpv6CbhePrimaryBlock header;
        boost::asio::const_buffer actualSerializedPrimaryBlockPtr;
        bool dirty;

        BPCODEC_EXPORT void SetManuallyModified();
    };
    struct Bpv6CanonicalBlockView {
        std::unique_ptr<Bpv6CanonicalBlock> headerPtr;
        boost::asio::const_buffer actualSerializedBlockPtr;
        bool dirty;
        bool markedForDeletion;

        BPCODEC_EXPORT void SetManuallyModified();
        BPCODEC_EXPORT void SetBlockProcessingControlFlagAndDirtyIfNecessary(const BPV6_BLOCKFLAG flag);
        BPCODEC_EXPORT void ClearBlockProcessingControlFlagAndDirtyIfNecessary(const BPV6_BLOCKFLAG flag);
        BPCODEC_EXPORT bool HasBlockProcessingControlFlagSet(const BPV6_BLOCKFLAG flag) const;
        
    };
    //typedef std::multimap<uint8_t, Bpv6CanonicalBlockView>::iterator canonical_block_view_iterator_t;
    //typedef std::pair<canonical_block_view_iterator_t, canonical_block_view_iterator_t> canonical_block_view_range_t;

    BPCODEC_EXPORT BundleViewV6();
    BPCODEC_EXPORT ~BundleViewV6();

    BPCODEC_EXPORT void AppendMoveCanonicalBlock(std::unique_ptr<Bpv6CanonicalBlock> & headerPtr);
    BPCODEC_EXPORT void PrependMoveCanonicalBlock(std::unique_ptr<Bpv6CanonicalBlock> & headerPtr);
    BPCODEC_EXPORT bool GetSerializationSize(uint64_t & serializationSize) const;
    BPCODEC_EXPORT std::size_t GetCanonicalBlockCountByType(const BPV6_BLOCK_TYPE_CODE canonicalBlockTypeCode) const;
    BPCODEC_EXPORT std::size_t GetNumCanonicalBlocks() const;
    BPCODEC_EXPORT void GetCanonicalBlocksByType(const BPV6_BLOCK_TYPE_CODE canonicalBlockTypeCode, std::vector<Bpv6CanonicalBlockView*> & blocks);
    BPCODEC_EXPORT std::size_t DeleteAllCanonicalBlocksByType(const BPV6_BLOCK_TYPE_CODE canonicalBlockTypeCode);
    BPCODEC_EXPORT bool LoadBundle(uint8_t * bundleData, const std::size_t size, const bool loadPrimaryBlockOnly = false);
    BPCODEC_EXPORT bool SwapInAndLoadBundle(std::vector<uint8_t> & bundleData, const bool loadPrimaryBlockOnly = false);
    BPCODEC_EXPORT bool CopyAndLoadBundle(const uint8_t * bundleData, const std::size_t size, const bool loadPrimaryBlockOnly = false);
    BPCODEC_EXPORT bool IsValid() const;
    BPCODEC_EXPORT bool Render(const std::size_t maxBundleSizeBytes);
    //bool RenderInPlace(const std::size_t paddingLeft);
    BPCODEC_EXPORT void Reset(); //should be private
private:
    BPCODEC_NO_EXPORT bool Load(const bool loadPrimaryBlockOnly);
    BPCODEC_NO_EXPORT bool Render(uint8_t * serialization, uint64_t & sizeSerialized);
    
public:
    Bpv6PrimaryBlockView m_primaryBlockView;
    const uint8_t * m_applicationDataUnitStartPtr;
    std::list<Bpv6CanonicalBlockView> m_listCanonicalBlockView; //list will maintain block relative order


    boost::asio::const_buffer m_renderedBundle;
    std::vector<uint8_t> m_frontBuffer;
    std::vector<uint8_t> m_backBuffer;
};

#endif // BUNDLE_VIEW_V6_H

