FROM ubuntu:24.04
COPY --from=build /build/apps/server/matching_engine/matching-engine /usr/local/bin/matching-engine
ENTRYPOINT ["matching-engine"]
