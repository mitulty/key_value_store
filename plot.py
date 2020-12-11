import os
import matplotlib.pyplot as plt
################## extreme case, when last line of the file is empty is not handled #################

class obtainGraph:
    def __init__(self,dir):
        parent_list = os.listdir(dir)
        count = 0
        parent_list.sort()
        self.x = []  # contains list x
        self.y = []  # contains list y

        for child in parent_list:
            f = open(str(dir)+str(child), "r")
            lines_list = f.read().splitlines()
            total_resposes = len(lines_list) - 1
            last_line = lines_list[-1]
            print("Last Line",last_line)
            px, py = total_resposes, last_line.split(" ")[2]
            self.x.append(px)
            self.y.append(float(py))


    def plot_graph(self):
        # ploting data
        # respose time vs load
        x, y = self.x, self.y
        fig,p =  plt.subplots(1,2,squeeze=False)
        p[0][0].scatter(x, y)
        p[0][0].set_xlabel('Load {number of requests}')
        p[0][0].set_ylabel('Response Time in seconds')
        p[0][0].set_title('Response time vs Load')

        #Througput vs load
        y = [px / py for py, px in zip(y, x)]
        p[0][1].scatter(x, y,color='red')
        p[0][1].set_xlabel('Load {number of requests}')
        p[0][1].set_ylabel('Throuput in Request per seconds')
        p[0][1].set_title('Throuput vs Load')

        plt.savefig("graph.png")
        plt.show()
        # print(x, y)



