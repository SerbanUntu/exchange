# Public API documentation

This document provides a high-level overview of the components and conventions of the exchange API. Interaction with the exchange is done only through gRPC. All the relevant `.proto` files are available in this folder.

## Modules

- **common.proto**: Shared types used across the other modules.
- **orders.proto**: Order lifecycle operations (submitting, cancelling, and amending), along with the enums describing order type, side and time-in-force.
- **market_data.proto**: Read-only market data: OHLCV candles, order book snapshots (L1/L2), the L3 continuous update stream, order state, and executions (fills), along with their request/response messages.
- **securities.proto**: Metadata about tradable instruments. Defines `Security` (id, symbol, name, description) and the request/response messages for looking securities up by id or symbol, or listing all of them.
- **gateway.proto**: The `Gateway` service, which exposes all of the above as RPCs. This is the entry point client applications talk to.

## Conventions

- Currency is EUR.
- Prices are in cents.
- Lot size is 1 share.
- Tick size is 1 cent.
- Candle base resolution is 1 second. A `resolution_seconds` of N means N seconds per candle.

## Usage details

- The order price and TIF fields are required for limit orders and ignored for market orders.
- The gateway cannot guarantee how the matching engine will react to a request related to an order, so it just acks (`OrderAck`) that it received the request and forwarded it to the matching engine, without a guarantee on the results. As a consequence, adding an order and then immediately querying it may return `NOT_FOUND`, even if it is eventually added. Clients should instead subscribe to the L3 data stream. Adding synchronization with the engine would make the whole architecture useless due to the latency introduced.
- `AmendOrderRequest` is only valid for GTC orders (400 otherwise).
- There are three possible outcomes for amending a GTC order: The new (smaller) quantity has already been filled, the new quantity is smaller and has not been (fully) filled, and the order maintains its position in the queue, or the quantity is increased or the price changes, which puts the order at the back of the queue.
- L2 data can have at most 10 levels. Any request asking for more than this will fail.
- Optional fields in market data requests are used for filtering.
- Candle time ranges are `[start_time, end_time)` (start is inclusive, end is exclusive).
