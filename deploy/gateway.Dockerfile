FROM ubuntu:24.04
COPY --from=build /build/apps/server/gateway/gateway /usr/local/bin/gateway
ENTRYPOINT ["gateway"]
