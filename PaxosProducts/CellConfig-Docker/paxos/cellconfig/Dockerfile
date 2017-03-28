FROM python:2.7-slim


RUN apt-get update && \
    apt-get install -y openssl libevent-2.0 && \
    rm -rf /var/lib/apt/lists/*

RUN useradd -m paxos

USER paxos

COPY paxos-cellcfg.debian-8.x86_64.tar.gz /tmp/

WORKDIR /home/paxos

RUN tar xzf /tmp/paxos-cellcfg.debian-8.x86_64.tar.gz && \
    cd dist && ./setup.sh && \
    rm -rf /home/paxos/dist

WORKDIR /home/paxos/PAXOS/manager

EXPOSE 7000

ENTRYPOINT ["bin/gunicorn"]
CMD ["-c", "conf/gunicorn.conf", "cellconfig.ui.web:app"]
