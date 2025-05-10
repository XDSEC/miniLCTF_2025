#!/bin/sh

echo $FLAG > /m1n1FL@G
echo "\nNext, let's tackle the more challenging misc/pyjail">> /m1n1FL@G
chmod 600 /m1n1FL@G
chown root:root /m1n1FL@G

chmod 4755 /usr/bin/find

useradd -m minilUser
export FLAG=""
chmod -R 777 /app
su minilUser -c "python /app/app.py"

