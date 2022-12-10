/**
 * @file FragmentSet.cpp
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

#include "FragmentSet.h"
#include "Logger.h"
#include "Sdnv.h"
#include <algorithm>
#include <boost/next_prior.hpp>

FragmentSet::data_fragment_t::data_fragment_t(uint64_t paramBeginIndex, uint64_t paramEndIndex) : beginIndex(paramBeginIndex), endIndex(paramEndIndex) { }
FragmentSet::data_fragment_t::data_fragment_t() : beginIndex(0), endIndex(0) { } //a default constructor: X()
FragmentSet::data_fragment_t::~data_fragment_t() { } //a destructor: ~X()
FragmentSet::data_fragment_t::data_fragment_t(const data_fragment_t& o) : beginIndex(o.beginIndex), endIndex(o.endIndex) { } //a copy constructor: X(const X&)
FragmentSet::data_fragment_t::data_fragment_t(data_fragment_t&& o) : beginIndex(o.beginIndex), endIndex(o.endIndex) { } //a move constructor: X(X&&)
FragmentSet::data_fragment_t& FragmentSet::data_fragment_t::operator=(const data_fragment_t& o) { //a copy assignment: operator=(const X&)
    beginIndex = o.beginIndex;
    endIndex = o.endIndex;
    return *this;
}
FragmentSet::data_fragment_t& FragmentSet::data_fragment_t::operator=(data_fragment_t && o) { //a move assignment: operator=(X&&)
    beginIndex = o.beginIndex;
    endIndex = o.endIndex;
    return *this;
}
bool FragmentSet::data_fragment_t::operator==(const data_fragment_t & o) const {
    return (beginIndex == o.beginIndex) && (endIndex == o.endIndex);
}
bool FragmentSet::data_fragment_t::operator!=(const data_fragment_t & o) const {
    return (beginIndex != o.beginIndex) || (endIndex != o.endIndex);
}
bool FragmentSet::data_fragment_t::operator<(const data_fragment_t & o) const { //operator < (no overlap no abut) 
    return ((endIndex+1) < o.beginIndex);
}
bool FragmentSet::data_fragment_t::SimulateSetKeyFind(const data_fragment_t & key, const data_fragment_t & keyInSet) {
    //equal defined as !(a<b) && !(b<a)
    return !(key < keyInSet) && !(keyInSet < key);
}

//class which allows searching ignoring whether or not the keys abut
bool FragmentSet::data_fragment_no_overlap_allow_abut_t::operator<(const data_fragment_no_overlap_allow_abut_t& o) const { //operator < (no overlap allow abut)
    return (endIndex < o.beginIndex);
}
FragmentSet::data_fragment_no_overlap_allow_abut_t::data_fragment_no_overlap_allow_abut_t(uint64_t paramBeginIndex, uint64_t paramEndIndex) :
    data_fragment_t(paramBeginIndex, paramEndIndex) { }

//class which allows everything except identical pairs
bool FragmentSet::data_fragment_unique_overlapping_t::operator<(const data_fragment_unique_overlapping_t& o) const { //operator < (no overlap allow abut)
    if (beginIndex == o.beginIndex) {
        return (endIndex < o.endIndex);
    }
    return (beginIndex < o.beginIndex);
}
FragmentSet::data_fragment_unique_overlapping_t::data_fragment_unique_overlapping_t(uint64_t paramBeginIndex, uint64_t paramEndIndex) :
    data_fragment_t(paramBeginIndex, paramEndIndex) { }

//return true if the set was modified, false if unmodified
bool FragmentSet::InsertFragment(std::set<data_fragment_t> & fragmentSet, data_fragment_t key) {
    bool modified = false;
    while (true) {
        std::pair<std::set<data_fragment_t>::iterator, bool> res = fragmentSet.insert(key);
        if (res.second == true) { //fragment key was inserted with no overlap nor abut
            return true;
        }
        //fragment key not inserted due to overlap or abut.. res.first points to element in the set that may need expanded to fit new key fragment
        const uint64_t keyInMapBeginIndex = res.first->beginIndex;
        const uint64_t keyInMapEndIndex = res.first->endIndex;
        if ((key.beginIndex >= keyInMapBeginIndex) && (key.endIndex <= keyInMapEndIndex)) { //new key fits entirely inside existing key (set needs no modification)
            return modified;
        }
        key.beginIndex = std::min(key.beginIndex, keyInMapBeginIndex);
        key.endIndex = std::max(key.endIndex, keyInMapEndIndex);
        fragmentSet.erase(res.first);
        modified = true;
    }
}

bool FragmentSet::ContainsFragmentEntirely(const std::set<data_fragment_t> & fragmentSet, const data_fragment_t & key) {
    std::set<data_fragment_t>::const_iterator res = fragmentSet.find(key);
    if (res == fragmentSet.cend()) { //not found (nothing that overlaps or abuts)
        return false;
    }
    const uint64_t keyInMapBeginIndex = res->beginIndex;
    const uint64_t keyInMapEndIndex = res->endIndex;
    return ((key.beginIndex >= keyInMapBeginIndex) && (key.endIndex <= keyInMapEndIndex)); //new key fits entirely inside existing key (set needs no modification)
}



bool FragmentSet::DoesNotContainFragmentEntirely(const std::set<data_fragment_t> & fragmentSet, const data_fragment_t & key) {
#if 1
    std::set<data_fragment_no_overlap_allow_abut_t> * setNoOverlapAllowAbut = (std::set<data_fragment_no_overlap_allow_abut_t> *) &fragmentSet;
    return (setNoOverlapAllowAbut->find(*((data_fragment_no_overlap_allow_abut_t*)&key)) == setNoOverlapAllowAbut->cend());
#else
    std::set<data_fragment_t>::const_iterator res = fragmentSet.find(key);
    if (res == fragmentSet.cend()) { //not found (nothing that overlaps or abuts)
        return true;
    }
    std::set<data_fragment_t>::const_iterator startIt = (res == fragmentSet.cbegin()) ? res : boost::prior(res);
    std::set<data_fragment_t>::const_iterator endIt = boost::next(res);
    for (std::set<data_fragment_t>::const_iterator it = startIt; it != fragmentSet.cend(); ++it) {
        const uint64_t keyInMapBeginIndex = it->beginIndex;
        const uint64_t keyInMapEndIndex = it->endIndex;
        if ((key.beginIndex > keyInMapEndIndex) || (key.endIndex < keyInMapBeginIndex)) { //new key does not fit entirely inside existing key
            continue;
        }
        else { //new key fits at least partially inside existing key
            return false;
        }
        if (it == endIt) {
            break;
        }
    }
    return true;
#endif
}

//return true if the set was modified, false if unmodified
bool FragmentSet::RemoveFragment(std::set<data_fragment_t> & fragmentSet, const data_fragment_t & key) {
    const uint64_t deleteBegin = key.beginIndex;
    const uint64_t deleteEnd = key.endIndex;
    if (deleteBegin > deleteEnd) { //invalid, stop
        return false; //unmodified
    }
    bool modified = false;
    for (std::set<data_fragment_t>::iterator it = fragmentSet.lower_bound(key); it != fragmentSet.end(); ) {
        const uint64_t keyInMapBeginIndex = it->beginIndex;
        const uint64_t keyInMapEndIndex = it->endIndex;
        if (deleteBegin > keyInMapEndIndex) { //stop
            return modified;
        }
        else if (deleteEnd < keyInMapBeginIndex) { //continue
            ++it;
            continue;
        }
        else if ((deleteBegin <= keyInMapBeginIndex) && (deleteEnd >= keyInMapEndIndex)) { //remove map key entirely
            std::set<data_fragment_t>::iterator itToErase = it;
            ++it;
            fragmentSet.erase(itToErase);
            modified = true;
        }
        else if ((deleteBegin > keyInMapBeginIndex) && (deleteEnd < keyInMapEndIndex)) { //split map key in 2 and return
            data_fragment_t replacementKeyLeft(keyInMapBeginIndex, deleteBegin - 1);
            data_fragment_t replacementKeyRight(deleteEnd + 1, keyInMapEndIndex);
            std::set<data_fragment_t>::iterator itToErase = it;
            ++it;
            if (itToErase != fragmentSet.begin()) { //can be optimized with hint
                std::set<data_fragment_t>::iterator itPrecedingInsertedElement = boost::prior(itToErase);
                fragmentSet.erase(itToErase);
                fragmentSet.insert(itPrecedingInsertedElement, replacementKeyRight); //insert right first so we don't have to change itPrecedingInsertedElement
                fragmentSet.insert(itPrecedingInsertedElement, replacementKeyLeft);
            }
            else { //partial optimization
                fragmentSet.erase(itToErase);
                std::set<data_fragment_t>::iterator itLeftPrecedingInsertedRight = fragmentSet.insert(replacementKeyLeft).first;
                fragmentSet.insert(itLeftPrecedingInsertedRight, replacementKeyRight);
            }
            modified = true;
        }
        else if ((deleteBegin <= keyInMapBeginIndex) && (deleteEnd >= keyInMapBeginIndex)) { //alter left only
            data_fragment_t replacementKey(deleteEnd + 1, keyInMapEndIndex);
            std::set<data_fragment_t>::iterator itToErase = it;
            ++it;
            if (itToErase != fragmentSet.begin()) { //can be optimized with hint
                std::set<data_fragment_t>::iterator itPrecedingInsertedElement = boost::prior(itToErase);
                fragmentSet.erase(itToErase);
                fragmentSet.insert(itPrecedingInsertedElement, replacementKey); 
            }
            else { //no optimization
                fragmentSet.erase(itToErase);
                fragmentSet.insert(replacementKey);
            }
            modified = true;
        }
        else if ((deleteBegin <= keyInMapEndIndex) && (deleteEnd >= keyInMapEndIndex)) { //alter right only
            data_fragment_t replacementKey(keyInMapBeginIndex, deleteBegin - 1);
            std::set<data_fragment_t>::iterator itToErase = it;
            ++it;
            if (itToErase != fragmentSet.begin()) { //can be optimized with hint
                std::set<data_fragment_t>::iterator itPrecedingInsertedElement = boost::prior(itToErase);
                fragmentSet.erase(itToErase);
                fragmentSet.insert(itPrecedingInsertedElement, replacementKey);
            }
            else { //no optimization
                fragmentSet.erase(itToErase);
                fragmentSet.insert(replacementKey);
            }
            modified = true;
        }
        else { //??
            ++it;
        }
    }
    return modified;
}


void FragmentSet::PrintFragmentSet(const std::set<data_fragment_t> & fragmentSet) {
    for (std::set<data_fragment_t>::const_iterator it = fragmentSet.cbegin(); it != fragmentSet.cend(); ++it) {
        LOG_INFO(hdtn::Logger::SubProcess::none) << "(" << it->beginIndex << "," << it->endIndex << ") ";
    }
}

void FragmentSet::GetBoundsMinusFragments(const data_fragment_t bounds, const std::set<data_fragment_t>& fragmentSet, std::set<data_fragment_t>& boundsMinusFragmentsSet) {
    boundsMinusFragmentsSet.clear();
    InsertFragment(boundsMinusFragmentsSet, bounds);
    for (std::set<data_fragment_t>::const_iterator it = fragmentSet.cbegin(); it != fragmentSet.cend(); ++it) {
        RemoveFragment(boundsMinusFragmentsSet, *it);
    }
}
