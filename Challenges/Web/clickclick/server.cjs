const express = require('express');
const bodyParser = require('body-parser');
const path = require("path");

const app = express();
const PORT = 8081;


const flag = 'miniLCTF{test_flag}';
const staticDir = path.join(__dirname, "dist");
app.use(express.static(staticDir));

// 中间件设置
app.use(bodyParser.json());
app.use(bodyParser.urlencoded({ extended: true }));

// 不安全的 merge 函数 - 会导致原型污染
function merge(target, source) {
    for (const key in source) {
        if (typeof target[key] === 'object' && typeof source[key] === 'object') {
            merge(target[key], source[key]);
        } else {
            target[key] = source[key];
        }
    }
    return target;
}
const point = {}

app.get('/', (req, res) => {
    res.sendFile(path.join(__dirname, "index.html"))
})

app.post('/update-amount', (req, res) => {
    
    if ( req.body.type != "set") {
        return res.status(400).send('并不是设置数值');
    }
    
    if ( req.body.point.amount >= 1000 ) {
        return res.status(400).send('你按的太快了！');
    }
    
    if ( req.body.point.amount < 0) {
        return res.status(403).send('hacker!!');
    }
    
    if ( req.body.point.amount == 0 || req.body.point.amount == null) {
        delete req.body.point.amount;
    }
    
    merge(point, req.body.point)
    
    let amount = point.amount;
    
    if (amount >= 10000) {
        return res.status(200).send(flag);
    } else {
        return res.status(200).send('ok');
    }
    
})


app.listen(PORT, () => {
    console.log(`服务器运行在 http://localhost:${PORT}`);
    console.log('可用端点:');
    console.log('/update-amount: ');
});