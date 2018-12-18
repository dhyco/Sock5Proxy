test
linux test
win test


version: "3"

services:
  nodeosd:
    image: eosio/eos-dev:v1.3.0
    command: nodeos -e -p eosio --plugin eosio::producer_plugin --plugin eosio::history_plugin --plugin eosio::chain_api_plugin --plugin eosio::history_api_plugin --plugin eosio::http_plugin -d /mnt/dev/data --config-dir /mnt/dev/config --http-server-address=0.0.0.0:8888 --access-control-allow-origin=* --contracts-console --http-validate-host=false
    hostname: nodeosd
    ports:
      - 8888:8888
      - 9876:9876
    expose:
      - "8888"
    volumes:
      - nodeos-data-volume:/opt/eosio/bin/data-dir
      - /tmp/eosio/work:/work
      - /tmp/eosio/data:/mnt/dev/data
      - /tmp/eosio/config:/mnt/dev/config
    cap_add:
      - IPC_LOCK
    stop_grace_period: 10m
    networks:
     - eosdev

  keosd:
    image: eosio/eos-dev:v1.3.0
    command: keosd --http-server-address=0.0.0.0:9876
    hostname: keosd
    volumes:
      - keosd-data-volume:/opt/eosio/bin/data-dir
    networks:
     - eosdev
    stop_grace_period: 10m


volumes:
  nodeos-data-volume:
    external: true
  keosd-data-volume:
    external: true

networks:
    eosdev:
       driver: bridg
