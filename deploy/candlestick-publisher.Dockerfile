FROM ubuntu:24.04
COPY --from=build /build/apps/server/candlestick_publisher/candlestick-publisher /usr/local/bin/candlestick-publisher
ENTRYPOINT ["candlestick-publisher"]
