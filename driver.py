import plot as plt
import sys

if(len(sys.argv)!=2):
    print("Enter the required arguments: Path_Of_The_Output_Folder")
    exit(0)
path = str((sys.argv)[1])
#dir_name = "./op_files/"
dir_name=path
graph = plt.obtainGraph(dir_name)
graph.plot_graph()
