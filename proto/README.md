# Public API documentation

This document provides a high-level overview of the components and conventions of the exchange API. Interaction with the exchange is done only through gRPC. All the relevant `.proto` files are available in this folder.

## Modules

- **common.proto**: Shared types used across the other modules.
- **orders.proto**: Order lifecycle operations (submitting, cancelling, and amending), along with the enums describing order type, side, time-in-force, and the possible outcomes of a cancel/amend.
- **market_data.proto**: Read-only market data: OHLC candles, order book snapshots (L1/L2/L3), and executions (fills), along with their request/response messages.
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
- `OrderResponse` updates are streamed upon the initial creation of the order. New messages are sent whenever more of its quantity gets filled, or when its state changes. Messages emitted due to a new fill will also contain a value in the `most_recent_fill` field.
- `AmendOrderRequest`/`AmendOrderResponse` are only valid for GTC orders (400 otherwise).
- There are three possible outcomes for amending a GTC order: The new (smaller) quantity has already been filled, the new quantity is smaller and has not been (fully) filled, and the order maintains its position in the queue, or the quantity is increased or the price changes, which puts the order at the back of the queue. These outcomes are reflected in the `AmendResult` enum.
- The `order_id` field for orders in the order book is only populated for L3. For L1, the bids and asks arrays contain a single value. For L2, they contain one entry per price level.
- Optional fields in market data requests are used for filtering.
- Candle time ranges are `[start_time, end_time)` (start is inclusive, end is exclusive).
