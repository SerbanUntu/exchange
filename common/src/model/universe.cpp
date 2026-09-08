#include "common/model/universe.hpp"

namespace exchange::common
{
const std::array<Security, Universe::NUMBER_OF_SECURITIES> &Universe::getSecurities()
{
    static const std::array<Security, NUMBER_OF_SECURITIES> securities = [] {
        Security nord{};
        nord.set_security_id(1);
        nord.set_symbol("NORD");
        nord.set_name("Nordis Group");
        nord.set_description("Luxury housing and resorts.");

        Security dnia{};
        dnia.set_security_id(2);
        dnia.set_symbol("DNIA");
        dnia.set_name("Dania");
        dnia.set_description("Middle eastern fast food chain.");

        Security ozno{};
        ozno.set_security_id(3);
        ozno.set_symbol("OZNO");
        ozno.set_name("Ozono");
        ozno.set_description("High-end dental clinic.");
        return std::array{nord, dnia, ozno};
    }();

    return securities;
}
} // namespace exchange::common