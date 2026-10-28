#pragma once
#include "server/common/model/event/event.hpp"

#include <array>
#include <cstdint>
#include <span>
#include <string>
#include <unordered_map>
#include <vector>

namespace exchange::server
{

inline constexpr std::array FUZZING_SECURITY_IDS{SecurityId{1}, SecurityId{2}};

/**
 * Decode a list of domain events from an array of bytes.
 *
 * @param bytes The serialized bytes.
 * @param securities The set of valid security ids.
 * @return A list of events decoded from the bytes.
 */
std::vector<Event> decodeEvents(std::span<const uint8_t> bytes, std::span<const SecurityId> securities);

/**
 * Encode a list of domain events into an array of bytes.
 *
 * @param events The events to encode.
 * @param securities The set of valid security ids.
 * @return A list of events encoded into bytes.
 */
std::vector<uint8_t> encodeEvents(std::span<const Event> events, std::span<const SecurityId> securities);

/**
 * For each file name-list of events pair, it writes the file with the serialized list of events into
 * the `./seeds` directory.
 *
 * @param seeds A mapping of file name to list of events.
 */
void serializeEventsIntoSeedsDirectory(const std::unordered_map<std::string, std::span<const Event>> &seeds);

} // namespace exchange::server