FROM ubuntu:24.04
COPY --from=build /build/apps/server/execution_publisher/execution-publisher /usr/local/bin/execution-publisher
ENTRYPOINT ["execution-publisher"]
