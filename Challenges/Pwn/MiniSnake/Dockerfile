FROM ubuntu:24.10

RUN useradd -m ctf
WORKDIR /

RUN sed -i "s/http:\/\/archive.ubuntu.com/http:\/\/mirrors.tuna.tsinghua.edu.cn/g" /etc/apt/sources.list && \
    apt-get update && apt-get -y dist-upgrade && \
    apt-get install -y socat libncurses6

COPY ./build/* /
RUN chown ctf:ctf minisnake flag && \
    chown root:root start.sh && \
    chmod 750 minisnake && \
    chmod 500 start.sh && \
    chmod 540 flag

CMD ["/start.sh"]

EXPOSE 9999
