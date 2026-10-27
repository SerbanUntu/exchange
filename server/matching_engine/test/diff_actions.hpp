#pragma once
#include "server/common/model/action/action.hpp"
#include "server/common/model/event/event.hpp"

#include <string>
#include <vector>

namespace exchange::server
{

std::string toString(const Action &action);

std::string toString(const Event &event);

std::string diffActions(const std::vector<Action> &expected, const std::vector<Action> &actual);

} // namespace exchange::server
