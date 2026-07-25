# Stock Exchange System Architecture Overview

## Data Flow (baseline)

```
Network
  │ gRPC request
  ▼
Gateway ──(uses)──► Risk Checks ──► reject / continue
  │ writes internal event (+ correlation id) to
  ▼
Event Stream  (MPSC: many Gateway threads -> one queue)
  │
  ▼
Matching Engine  (single instance, owns every security's order book)
  │ fire-and-forget to
  ├──────────────────► Archiver (embedded thread) ──► disk, later exported to S3 Glacier
  │ writes actions to
  ▼
Action Stream  (SPMC: one queue -> many consumers)
  ├──► Order Manager             (per-order state; resolves the Gateway's blocking
  │                                Submit/Cancel/Amend calls; pushes WatchOrder updates)
  ├──► Orderbook Data Publisher  (current book state: L1/L2 eager, L3 on futex notify)
  ├──► Execution Data Publisher  (fill / execution history)
  └──► Candlestick Data Publisher (OHLC candles)
         │ uses
         ▼
       Archiver (embedded thread) ──► disk, later exported to S3 Glacier

Gateway reads from / blocks on Order Manager and the three publishers over shared memory
to answer gRPC calls, then responds to Network.
```

- Process separation
    - By default, each component is a separate OS process (exceptions are below).
    - The Archiver is a separate thread in each consumer process that needs to archive its incoming stream (currently the Matching Engine, for the Event Stream, and the Candlestick Data Publisher, for the Action Stream). It communicates with the host's main thread via an SPSC queue and is always called asynchronously (fire-and-forget) so it never stalls the host process. A separate, standalone Archiver process also ingests the Action Stream directly and performs the same archiving, independent of any single publisher's uptime.
    - The Risk Checks module is a basic stateless library called by the Gateway before forwarding to the Event Stream. It is not a separate process.
- Inter-process communication (IPC)
    - IPC is done via shared memory.
    - The gateway either notifies the publisher that it needs data using a futex or relies on the publisher to update the shared buffer eagerly.
    - Eager shared memory updates are done only for L1 and L2 market data.
- The Matching Engine acts as a bottleneck, with the MPSC Event Stream queue as the input and the SPMC Action Stream queue as the output.
- Persistence and Restart
    - The contents of the Event Stream and the Action Stream, as well as the descriptor each process has on them, are flushed to disk on safe shutdown and read back on startup.
    - When the shutdown command is issued, the Gateway stops accepting new requests.
    - The data of the stateful components (publishers) is also flushed to disk and recovered.
- Authentication
    - Uses JWT (`jwt-cpp` library).
- Request/response correlation (order IDs)
    - Every internal event the Gateway writes to the Event Stream carries a Gateway-assigned correlation id (e.g., gateway thread id, security id, and a monotonically increasing sequence number).
    - This acts as the order ID.

## Gateway (stateless)

- FROM Network – gRPC requests.
- FROM Order Manager – Requested order data.
- FROM Orderbook Data Publisher – Requested market data.
- FROM Execution Data Publisher – Requested order data.
- FROM Candlestick Data Publisher – Requested market data.
- TO Order Manager – Notifies (via futex) that order data is needed.
- TO Orderbook Data Publisher – Notifies (via futex) that L3 order book data is needed.
- TO Candlestick Data Publisher – Notifies (via futex) that market candle data is needed.
- TO Execution Data Publisher – Notifies (via futex) that order execution data is needed.
- TO Event Stream – gRPC requests stored as internal events, with account number (obtained from the JWT) and a Gateway-assigned correlation id.
- TO Network – gRPC responses, obtained by querying the order manager and publishers.
- USES Risk Checks – To reject orders that do not pass the checks.
- Responsibilities: gRPC service, Auth, Validation, Normalization. Each parallel thread writes to the Event Stream MPSC queue.

## Risk Checks (stateless)

- USED BY Gateway – To reject orders that do not pass the checks.
- Responsibilities: Quantity and price limits (sanity checks).

## Matching Engine (stateless)

- FROM Event Stream – Internal events.
- TO Action Stream – Internal order book changes (actions).
- USES Archiver – Providing consumption of the Event Stream.
- Responsibilities: Keeping the order book data in memory (one book per `security_id`) and turning events into actions on the order book. Single-threaded, deterministic, pinned to one CPU core. Performs Self-Trade Prevention (STP) by canceling the incoming (taker) orders.

## Order Manager (stateful)

- FROM Action Stream – Actions produced by the Matching Engine.
- TO Gateway – Requested order data.
- Responsibilities: Sending updates about individual orders to the actors who placed them.

## Orderbook Data Publisher (stateful)

- FROM Action Stream – Actions produced by the Matching Engine.
- TO Gateway – Requested market data.
- Responsibilities: Publishing the current orderbook state.

## Candlestick Data Publisher (stateful)

- FROM Action Stream – Actions produced by the Matching Engine.
- TO Gateway – Requested market data.
- USES Archiver – Historical market candle data.
- Responsibilities: Publishing the historical market candle data.

## Execution Data Publisher (stateful)

- FROM Action Stream – Actions produced by the Matching Engine.
- TO Gateway – Requested order data.
- Responsibilities: Publishing the historical execution data for an order or multiple orders.

## Archiver (stateless)

- FROM Action Stream – Actions produced by the Matching Engine.
- TO AWS S3 Glacier – Exported historical data.
- USED BY Matching Engine – Ingesting the Event Stream (events from the outside world (e.g., order placed/canceled)). Called in a fire-and-forget way to not stall the engine.
- USED BY Candlestick Data Publisher – Historical market candle data.
- Responsibilities: Sends historical data to cold storage (e.g., AWS) for compliance and inspection. Is on a separate thread and receives events in a fire-and-forget way to not stall the publishers.

# Extensions (out-of-scope requirements)

- High Availability (four nines)
    - Scale stateless processes horizontally, and stateful ones vertically (at first at least).
    - Put the stateless Gateway behind a load balancer with health checks. Active connections should be drained before removing an instance.
    - Use warm backup instances for components (more aggressively for the matching engine).
    - Track an SLO per component and issue alerts when the availability drops to or below four nines.
- Fault Tolerance
    - For network failures, add a Sequencer module to publishers and gateway that keeps track of sequence numbers.
    - Modify the API to allow clients to provide sequence numbers and to include sequence numbers as part of gateway responses, so the client can detect missed updates.
    - For process failures, configure systemd `.service` files with auto-restart, provide log files and thread dumping mechanisms.
    - Each process should send heartbeats to a central monitoring process to detect hangs. The monitoring process will terminate hanging processes.
    - For a whole system crash, configure restarts and set up automatic backups in multiple AZs, use Raft to determine availability and consensus amongst the multiple redundant instances.
- FIX Support
    - The Gateway could accept FIX messages and convert them to FIX over Simple Binary Encoding (SBE) for faster processing and storage.
    - Aligns more with financial industry.
- Network Security
    - Rate limiting
    - Firewall
    - URL Hardening (do not allow users to specify huge ranges in the query parameters)
    - Effective caching / CDN.
    - Isolating public from private services and endpoints.
- Multicast
- Cloud Deployment (AWS)
    - mTLS between internal services if they are split across hosts (e.g., a private CA or SPIFFE/SPIRE), with automatic short-lived certificate rotation.
    - Centralized secrets management (e.g., HashiCorp Vault or AWS KMS) for JWT signing keys and certificates.
    - Private subnets so only the Gateway is reachable.
    - Infrastructure-as-code (e.g., Terraform) for reproducible environments. Image scanning in CI.
- Trading Schedule (opening and closing times)
- More advanced Risk Checks
    - Price bands (huge deviations from previous orders).
    - Entitlements (whether an account is allowed to trade on a particular instrument). Could implement granular permissions (e.g., per order type).
- Wallet management
    - The balance check can be part of the Risk Checks process.
    - The exchange could keep a Transaction Ledger component as part of a larger post-trade settlement process (another consumer of the Action Stream).
- Frontend (read-only)
    - A React dashboard that provides the real-time view of the order book and candlestick charts.
    - Would require extending the API to provide streaming publisher data.
    - Acts as an additional client that connects to the Gateway.
- Frontend (interactive)
    - Additional screens for order management and lifecycle, and execution history. 
- Multi-currency support and conversion
- Regulatory compliance and reporting (CAT, MiFID II)
- Other market features
    - Background market monitoring (detecting manipulation)
    - Post-only orders (reject if the order would take liquidity)
    - Stop and stop-limit orders
    - Iceberg orders (partially visible, have a separate display quantity)
    - Pegged orders
    - Options, futures
    - Implied matching
    - AON resting orders
- Other operational features
    - Kill-switch for the entire exchange
