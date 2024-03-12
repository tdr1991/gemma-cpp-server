import socket
from multiprocessing import Process

SERVER_IP = '127.0.0.1'
SERVER_PORT = 8080
BUFFER_SIZE = 1024

def send(message):
    # 创建套接字
    client_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

    # 连接到服务器
    client_socket.connect((SERVER_IP, SERVER_PORT))
    print("Connected to server. {}".format(message))
    
    # 发送消息
    client_socket.sendall(message.encode())

    # 接收响应
    response = client_socket.recv(BUFFER_SIZE)
    print("Received:", response.decode())

    # 关闭套接字
    client_socket.close()
    
def main():
    p_list = []
    q_list = ["hi", "who are you", "1+1", "你有哪些能力", "支持哪些语言"]
    for i in range(5):
        p = Process(target=send, args=(q_list[i],))
        p.start()
        p_list.append(p)
    for p in p_list:
        p.join()

if __name__ == "__main__":
    main()
