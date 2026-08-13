add_library(exchange-server-common STATIC)
add_library(exchange::server::common ALIAS exchange-server-common)
target_sources(exchange-server-common
        PRIVATE
        src/collections/orderbook/orderbook.cpp
        PUBLIC
        FILE_SET HEADERS
        BASE_DIRS include
        FILES
        include/server/common/collections/orderbook/matched_order.hpp
        include/server/common/collections/orderbook/order_node.hpp
        include/server/common/collections/orderbook/orderbook.hpp
        include/server/common/collections/orderbook/price_node.hpp
        include/server/common/model/action/action.hpp
        include/server/common/model/action/action_type.hpp
        include/server/common/model/action/order_added_action.hpp
        include/server/common/model/action/order_amended_action.hpp
        include/server/common/model/action/order_cancelled_action.hpp
        include/server/common/model/action/order_executed_action.hpp
        include/server/common/model/action/order_matched_action.hpp
        include/server/common/model/event/amend_limit_order_event.hpp
        include/server/common/model/event/cancel_limit_order_event.hpp
        include/server/common/model/event/event.hpp
        include/server/common/model/event/event_type.hpp
        include/server/common/model/event/new_limit_order_event.hpp
        include/server/common/model/event/new_market_order_event.hpp
        include/server/common/model/value_object/account_id.hpp
        include/server/common/model/value_object/order_id.hpp
        include/server/common/model/value_object/price.hpp
        include/server/common/model/value_object/quantity.hpp
        include/server/common/model/value_object/security_id.hpp
        include/server/common/model/order_type.hpp
        include/server/common/model/side.hpp
        include/server/common/model/time_in_force.hpp
)

target_link_libraries(exchange-server-common
        PUBLIC
        exchange::common
        boost::boost
)
