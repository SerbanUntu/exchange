FROM ubuntu:24.04
COPY --from=build /build/apps/server/archiver_process/archiver-process /usr/local/bin/archiver-process
ENTRYPOINT ["archiver-process"]
