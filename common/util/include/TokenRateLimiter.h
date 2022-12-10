/**
 * @file TokenRateLimiter.h
 * @author  Brian Sipos
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
 * The TokenRateLimiter class is used in rate limiting the sending of UDP packets.
 */

#ifndef TOKEN_RATE_LIMITER_H
#define TOKEN_RATE_LIMITER_H 1

#include <boost/date_time/posix_time/posix_time_types.hpp>
#include <cstdint>
#include "hdtn_util_export.h"


/** Implement a constant rate Token Bucket.  It uses signed numbers and can go negative (if there is at least 1 token available to the borrower)
 *
 * The expected workflow is to:
 * 1. Define the limit with setRate()
 * 2. For each possible event:
 *    a. Call AddTime() based on the time since last use.
 *    b. Call GetRemainingTokens() to see if the wanted use is possible.
 *    c. If possible, call TakeTokens() to reduce the bucket.
 */
class TokenRateLimiter {
public:

    /// Initialize rate and count to zero
    HDTN_UTIL_EXPORT TokenRateLimiter();

    HDTN_UTIL_EXPORT ~TokenRateLimiter();

    /** Set the token fill rate.
     *
     * @param tokens The number of tokens to add per interval.
     * @param interval The interval of time to add @c tokens over.
     * @param window The window of time for averaging the rate over.
     * Used to set the maximum fill of the bucket, which limits allowed
     * burst rate (all tokens within the window can be exhausted at once).
     * @post The token count is initialized to what would be accumulated
     * over the burst interval.
     */
    HDTN_UTIL_EXPORT void SetRate(const int64_t tokens, const boost::posix_time::time_duration & interval,
        const boost::posix_time::time_duration & window);

    /** Tick the rate limiter.
     *
     * @param interval The interval of time to add tokens for based on
     * the limiter rate.
     */
    HDTN_UTIL_EXPORT void AddTime(const boost::posix_time::time_duration & interval);

    /** Get the current number of tokens remaining.
     *
     * @return The current count available.
     */
    HDTN_UTIL_EXPORT int64_t GetRemainingTokens() const;

    /** Get whether or not the token bucket is at full capacity of tokens.
     *
     * @return True if the token bucket is at full capacity of tokens, or false if tokens can still be added.
     */
    HDTN_UTIL_EXPORT bool HasFullBucketOfTokens() const;

    /** Decrement from the bucket.
     *
     * @param tokens The number of tokens to take away.
     * @return True if there were enough remaining to satisfy the need.
     */
    HDTN_UTIL_EXPORT bool TakeTokens(const uint64_t tokens);

    /** Get whether or not the token bucket has tokens to take (contains a positive number of tokens).
     *
     * @param tokens The number of tokens to take away.
     * @return True if there are a positive number of tokens.
     */
    HDTN_UTIL_EXPORT bool CanTakeTokens() const;

private:
    /// Number of tokens to accumulate over #m_rateInterval
    int64_t m_rateTokens;
    /// Interval to scale #m_remain to a token count
    boost::posix_time::time_duration m_rateInterval;
    /// Maximum #m_limit for burst situations
    int64_t m_limit;

    /// Denormalized count in units of (tokens*m_rateInterval)
    int64_t m_remain;

};

#endif // TOKEN_RATE_LIMITER_H
