FROM ubuntu:24.04
COPY --from=build /build/apps/server/orderbook_publisher/orderbook-publisher /usr/local/bin/orderbook-publisher
ENTRYPOINT ["orderbook-publisher"]
