FROM alpine AS build
RUN apk add --no-cache e2fsprogs-dev gcc make musl-dev
COPY . /src/
WORKDIR /src
ENV prefix=/usr
RUN ./configure && make -j3 && make install

FROM alpine
RUN apk add --no-cache e2fsprogs e2fsprogs-libs libcom_err
COPY --from=build /usr/bin/e2fsimage /usr/bin/e2fsimage
ENTRYPOINT ["/usr/bin/e2fsimage"]
