FROM python:3.11-alpine
RUN apk add --no-cache findutils
WORKDIR /app
COPY start.sh /
COPY app.py /app/
EXPOSE 5000

CMD ["/start.sh"]