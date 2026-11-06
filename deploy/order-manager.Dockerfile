FROM ubuntu:24.04
COPY --from=build /build/apps/server/order_manager/order-manager /usr/local/bin/order-manager
ENTRYPOINT ["order-manager"]
