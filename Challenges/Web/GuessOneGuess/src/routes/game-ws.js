module.exports = function(io) {
    io.on('connection', (socket) => {
        let targetNumber = Math.floor(Math.random() * 100) + 1;
        let guessCount = 0;
        let totalScore = 0;
        const FLAG = process.env.FLAG || "miniL{THIS_IS_THE_FLAG}";
        console.log(`新连接 - 目标数字: ${targetNumber}`);

        socket.emit('game-message', {
            type: 'welcome',
            message: '猜一个1-100之间的数字！',
            score: totalScore
        });

        socket.on('guess', (data) => {
            try {
              console.log(totalScore);
                const guess = parseInt(data.value);

                if (isNaN(guess)) {
                    throw new Error('请输入有效数字');
                }

                if (guess < 1 || guess > 100) {
                    throw new Error('请输入1-100之间的数字');
                }

                guessCount++;

                if (guess === targetNumber) {
                    const currentScore = Math.floor(100 / Math.pow(2, guessCount - 1));
                    totalScore += currentScore;

                    let message = `🎉 猜对了！得分 +${currentScore} (总分数: ${totalScore})`;
                    let showFlag = false;

                    if (totalScore > 1.7976931348623157e308) {
                        message += `\n🏴 ${FLAG}`;
                        showFlag = true;
                    }

                    socket.emit('game-message', {
                        type: 'result',
                        win: true,
                        message: message,
                        score: totalScore,
                        showFlag: showFlag,
                        currentScore: currentScore
                    });

                    targetNumber = Math.floor(Math.random() * 100) + 1;
                    console.log(`新目标数字: ${targetNumber}`);
                    guessCount = 0;
                } else {
                    if (guessCount >= 100) {
                      console.log("100次未猜中！将扣除当前分数并重置");
                        socket.emit('punishment', {
                            message: "100次未猜中！将扣除当前分数并重置",
                        });
                        return;
                    }
                    socket.emit('game-message', {
                        type: 'result',
                        win: false,
                        message: guess < targetNumber ? '太小了！' : '太大了！',
                        score: totalScore
                    });
                }
            } catch (err) {
                socket.emit('game-message', {
                    type: 'error',
                    message: err.message,
                    score: totalScore
                });
            }
        });
        socket.on('punishment-response', (data) => {
            console.log(data.score);
          totalScore -= data.score;
          console.log(totalScore);
          guessCount = 0;
          targetNumber = Math.floor(Math.random() * 100) + 1;
          console.log(`新目标数字: ${targetNumber}`);
          socket.emit('game-message', {
            type: 'result',
            win: true,
            message: "扣除分数并重置",
            score: totalScore,
            showFlag: false,
          });

        });
    });
};