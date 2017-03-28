FROM debian:jessie

RUN apt-get update && \
    apt-get install -y git gcc make bison patch unzip libreadline-dev \
            libssl-dev libffi-dev python-dev libevent-2.0 libevent-dev \
            curl && \
    rm -rf /var/lib/apt/lists/*

RUN cd /tmp/ && \
    git clone https://github.com/tritechpaxos/PaxosSolutions.git && \
    cd PaxosSolutions/PaxosProducts/CellConfig && \
    ./build_pkg.sh

CMD ["/bin/cat", "/tmp/PaxosSolutions/PaxosProducts/CellConfig/dist/paxos-cellcfg.debian-8.x86_64.tar.gz"]
