
import argparse
import matplotlib
import numpy as np
import os.path as op
from itertools import izip_longest, cycle, islice

matplotlib.use('PDF')


from cycler import cycler
#from sklearn import datasets
import matplotlib.pyplot as plt
from matplotlib.backends.backend_pdf import PdfPages
from matplotlib.ticker import ScalarFormatter


def carga_fichero(file_name, delim, header, indice, columna):

  salida=list()
  datos=np.genfromtxt(file_name, skip_header=header,
       comments="us.")
  datos= datos[:,1]
#  new_index, new_column = 0, 1
#  for i in np.unique(datos[:,new_index]):
#    mediana=np.median(datos[datos[:,new_index] == i, new_column])
#    salida.append([i, mediana])
  return datos

##############################################################################
# Main script
#############################################################################
################################################
#Configuration variables
################################################
titlefs = 20
ylabelfs = 20
xlabelfs = 20
xticksfs = 18
yticksfs = 18
legendfs = 16
linew = 3
markers = 12
fig_width = 8
fig_height = 6

colorcycle = ['#a1dab4', '#41b6c4', '#2c7fb8', '#253494', '#4f345a', '#8fa998' ]

def main():
  parser = argparse.ArgumentParser(description='Plot scheduler data.')
  parser.add_argument('fname', help='File prefix for reading the input data')
  parser.add_argument('--dir', help='Directory containing the input data.')
  title_name=""
  figure_name="CMMSE_Pwd.png"
  
  fig, ax = plt.subplots(1,1, figsize=(2.5,2))
  width= 1
  x=1

  bar_cycle = (cycler('hatch', ['///', '--', '...','\///', 'xxx', '\\\\']) * cycler('color', 'w')*cycler('zorder', [10]))
  styles = bar_cycle()
  

  power=np.array([[0.77,0.12,0.16],[0.077,0.713,0.210],[0.372,0.447,0.181],[0.15,0.701,0.148]]) 
  power=power.transpose()
  print power
  x=np.array([1,6,11,16])
  print x
  for i in range (0,3)  :
   h= ax.bar(x+i*1.25,power[i,:],fill=False,**next(styles)) 
   
  ax.legend(['CPU', 'GPU', 'FPGA'])
   # x=x+width*2

  text_labels= ('    ','','Mersenne Twister','      ','    Matrix Multiplication   ','  ','Watermark','','Sobel Filter')

 # plt.xticks(index,text_labels)
  ax.set_xticklabels(text_labels)
  plt.grid(True)
  ax.grid(linestyle='--', linewidth=1)
  ax.set_ylabel('Computational Power Normalized')
  ax.set_title(title_name)
  plt.savefig(figure_name) 
  
  plt.show()

if __name__ == "__main__":
  main() 
