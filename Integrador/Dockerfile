FROM alpine as build-env
RUN apk add --no-cache build-base
WORKDIR /app
COPY server.c .
COPY functions.h .
RUN gcc -o server server.c

FROM alpine
COPY --from=build-env /app/server /app/server
WORKDIR /app
CMD ["/app/server"]
