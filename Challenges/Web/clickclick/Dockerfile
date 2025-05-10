# 使用官方 Node 镜像
FROM node:alpine

# 设置工作目录
WORKDIR /app



# 复制必要的文件
COPY dist /app/dist
COPY server.cjs /app/server.cjs
COPY start.sh /app/start.sh
COPY node_modules /app/node_modules

# 安装依赖（只需要生产依赖）
# RUN npm config set registry https://registry.npmmirror.com
RUN npm config set proxy http://192.168.0.254:7890
RUN npm config set https-proxy http://192.168.0.254:7890
RUN npm install express 

# 暴露端口
EXPOSE 8081

# 启动服务器
# CMD ["npx", "serve", "-s", "dist", "-l", "3000"]
CMD ["sh", "start.sh"]