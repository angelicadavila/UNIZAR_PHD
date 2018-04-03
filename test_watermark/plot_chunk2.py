#!/usr/bin/env python

##############################################################################
# Adapted from: Antonio Vilches
# Revised by:   Andres Rodriguez for ViVid and Dario Suarez Gracia for CFD
# Version:      1.1
# Date:         7/32/2015
# Description:  Plotting script fro ViVid on Odroid
# Copyright:    Department Computer Architecture at University of Malaga
##############################################################################



import argparse
import matplotlib
import numpy as np
import os.path as op
from itertools import izip_longest, cycle, islice

matplotlib.use('PDF')

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
  file_test='modelt_dynamic.txt'
  title_name="Watermark dynamic data size=1.25Gb "
  figure_name="Watermarkwork_im2.png"
  datos=carga_fichero(file_test,'executionKernel: ',0,1,0)   

# hetero_marks = ['v-', '<-', '^-','>-']
# cpu_marks = ['s-','d-', 'h-']
# colors from https://matplotlib.org/examples/color/named_colors.html
#  hetero_color = ['lightgreen', 'green', 'mediumseagreen', 'mediumaquamarine']
#  cpu_color = [ 'blue', 'royalblue', 'deepskyblue']
  ################################################
  # tiempos de ejecuciOn
  ################################################

  samples=10
  dev=7
  
  exm=100

  datos2=np.reshape(datos,(exm*dev,samples))
  data_mean=np.average(datos2,axis=1)
  data_std=np.std(datos2,axis=1)
  print data_mean 
  print data_std
#  
  t=np.arange(0,500,5)
  #t=[0, 5, 10, 15, 20, 25, 30]
  # t=[3907430*4, 1953715*4, 1302476*4, 976857*4, 781486*4, 651238*4]
  #55820434*4 ]
  #[4096*4, 40960*4, 204800*4, 409600*4, 4096000*4]
   #width= 0.25
  text_label=['FPGA', 'GPU', 'CPU','FGC','FG','FC','GC']
  colors = plt.cm.tab20b(np.linspace(0,1 , dev))
  #colors = plt.cm.Pastel1(np.linspace(0,0.8 , dev))
  hetero_marks = ['v-', '<-', '^-','>-','s-','d-','h-']
  fig, ax = plt.subplots() 
  
  for x in range(dev):
    plt.plot(t,data_mean[exm*x:exm*(x+1)],hetero_marks[x], label=text_label[x],color=colors[x])
  #plt.xscale('log')
  #colors = plt.cm.Pastel1(np.linspace(0,0.8 , exm))
  ##colors = plt.cm.viridis(np.linspace(0,0.8 , exm))
  plt.legend(bbox_to_anchor=(1.04,1),loc='upper left')
  #plt.bar(index,mean_datos2,width,color=colors,yerr=std_datos2)
  #plt.xticks(index,text_labels)
  plt.grid(True) 
 
  # Adjust layout to make room for the table:
#  plt.subplots_adjust(left=0.2, bottom=0.2)
 
  ax.set_ylabel('Time(us)')
  ax.set_xlabel('Number of chunks')
  ax.set_title(title_name)
  plt.savefig(figure_name,bbox_inches="tight") 
  plt.show()




if __name__ == "__main__":
  main() 
