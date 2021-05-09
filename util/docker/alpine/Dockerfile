FROM alpine:3.13

RUN addgroup -g 11211 resql && adduser -D -u 11211 -G resql resql

ENV RESQL_VERSION 0.1.1-latest

RUN set -x \
	\
	&& apk add --no-cache --virtual .build-deps \
		ca-certificates \
		coreutils \
		gcc \
		libc-dev \
		linux-headers \
		make \
		cmake \
	\
	&& wget -O resql.tar.gz "https://github.com/tezc/resql/archive/v$RESQL_VERSION.tar.gz" \
	&& mkdir -p /usr/src/resql \
	&& tar -xzf resql.tar.gz -C /usr/src/resql --strip-components=1 \
	&& rm resql.tar.gz \
	\
	&& cd /usr/src/resql \
	&& ./build.sh \
	&& ./build.sh --install \
	\
	&& cd / && rm -rf /usr/src/resql \
	\
	&& runDeps="$( \
		scanelf --needed --nobanner --format '%n#p' --recursive /usr/local \
			| tr ',' '\n' \
			| sort -u \
			| awk 'system("[ -e /usr/local/lib/" $1 " ]") == 0 { next } { print "so:" $1 }' \
	)" \
	&& apk add --no-network --virtual .resql-rundeps $runDeps \
	&& apk del --no-network .build-deps \
	&& resql --version

RUN mkdir /data && chown resql:resql /data
VOLUME /data
WORKDIR /data

COPY docker-entrypoint.sh /usr/local/bin/
ENTRYPOINT ["docker-entrypoint.sh"]

USER resql
EXPOSE 7600
CMD ["resql", "--node-bind-url=tcp://0.0.0.0:7600"]