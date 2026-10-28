#pragma once
#include <cstdint>
#include <span>
#include <filesystem>

namespace exchange::server
{

/**
 * A lower-level function called indirectly by the fuzzer that decodes the array of bytes
 * into events and runs the matching engine on them.
 *
 * @param bytes The encoded events to run on the matching engine.
 */
void fuzzMatchingEngine(std::span<const uint8_t> bytes);

/**
 * Read the binary file and call `fuzzMatchingEngine` with its contents;
 *
 * @param path The path of the binary file.
 */
void runBinarySeed(const std::filesystem::path &path);

} // namespace exchange::server