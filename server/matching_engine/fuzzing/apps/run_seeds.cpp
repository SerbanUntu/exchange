#include "server/matching_engine/fuzzing/fuzzing.hpp"

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <exception>
#include <filesystem>
#include <fstream>
#include <ios>
#include <iterator>
#include <span>
#include <vector>

int main(const int argc, char **argv)
{
    try
    {
        for (const std::filesystem::path path : std::span(argv, static_cast<std::size_t>(argc)).subspan(1))
        {
            if (std::filesystem::is_directory(path))
            {
                for (const auto &entry : std::filesystem::directory_iterator(path))
                {
                    exchange::server::runBinarySeed(entry.path());
                }
            }
            else
            {
                exchange::server::runBinarySeed(path);
            }
        }
        return 0;
    }
    catch (const std::exception &err)
    {
        std::fprintf(stderr, "Failed to run the inputs: %s\n", err.what());
        return 1;
    }
}
