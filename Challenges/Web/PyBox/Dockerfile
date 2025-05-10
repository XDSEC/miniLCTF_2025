FROM python:3.11-slim
LABEL authors="xt"

WORKDIR /app
COPY app.py /app
COPY entrypoint.sh /
RUN chmod +x /entrypoint.sh

RUN pip install --no-cache-dir flask

EXPOSE 5000

CMD ["/entrypoint.sh"]