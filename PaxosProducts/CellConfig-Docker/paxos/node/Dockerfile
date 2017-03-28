FROM rastasheep/ubuntu-sshd:16.04

RUN useradd -m paxos
RUN echo 'paxos:paxos' | chpasswd

RUN apt-get update && \
    apt-get install -y libevent-2.0 && \
    rm -rf /var/lib/apt/lists/*

