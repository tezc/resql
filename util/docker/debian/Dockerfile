FROM debian:buster-slim

RUN groupadd --system --gid 11211 resql && useradd --system --gid resql --uid 11211 resql

ENV RESQL_VERSION master

RUN set -x \
	\
	&& savedAptMark="$(apt-mark showmanual)" \
	&& apt-get update \
	&& apt-get install -y --no-install-recommends \
		ca-certificates \
		dpkg-dev \
		gcc \
		libc6-dev \
		cmake \
		make \
		wget \
	&& rm -rf /var/lib/apt/lists/* \
	\
	&& wget -O resql.tar.gz "https://github.com/tezc/resql/archive/$RESQL_VERSION.tar.gz" \
	&& mkdir -p /usr/src/resql \
	&& tar -xzf resql.tar.gz -C /usr/src/resql --strip-components=1 \
	&& rm resql.tar.gz \
	\
	&& cd /usr/src/resql \
	&& ./build.sh \
	&& ./build.sh --install \
	&& resql --version

RUN mkdir /data && chown resql:resql /data
VOLUME /data
WORKDIR /data

COPY docker-entrypoint.sh /usr/local/bin/
ENTRYPOINT ["docker-entrypoint.sh"]

USER resql
EXPOSE 7600
CMD ["resql", "--node-bind-url=tcp://0.0.0.0:7600"]
