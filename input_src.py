import random
import string
import os
import subprocess
import threading
import sys

def request_server(path,file_index):
    subprocess.call(['bash', './run.sh',path,str(file_index)])


def get_random_string(length):
    letters = string.ascii_lowercase
    result_str = ''.join(random.choice(letters) for i in range(length))
    return result_str


def create_files(n):
    x = 100
    for i in range(1, n+1):
        f = open("./inp_files/in"+str(i)+".txt", "w")
        for j in range(x):
            y = random.randint(0, len(key)-1)
            z = random.randint(1, 3)
            if(z == 1):
                f.write(str(z)+","+key[y])
            elif(z == 2):
                f.write(str(z)+","+key[y]+","+value[y])
            else:
                f.write(str(z)+","+key[y])
            f.write("\n")
        f.close()
        x += 50


# print ('Number of arguments:', len(sys.argv), 'arguments.')
# print ('Argument List:', list(sys.argv))
if(len(sys.argv)!=3):
    print("Enter the required arguments: Number_Of_Clients Path_Of_The_server.conf")
    exit(0)
n = int((sys.argv)[1])
path=str(sys.argv[2])
os.system('make empty')
key = []
value = []
for i in range(1, 251):
    x = random.randint(10, 30)
    key.append(get_random_string(x))
for i in range(1, 251):
    x = random.randint(50, 90)
    value.append(get_random_string(x))

create_files(n)
# subprocess.call(['bash','./run.sh',str(n)])


threadLock = threading.Lock()
thread_list = []

for i in range(n):
    thread = threading.Thread(target=request_server, args=(path,i+1,), daemon=True)
    thread.start()
    thread_list.append(thread)

# Wait for all threads to complete
for t in thread_list:
    t.join()

