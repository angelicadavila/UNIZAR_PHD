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
  title_name="Watermark dynamic work size"
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

  samples=5
  dev=3
  
  exm=7

  datos2=np.reshape(datos,(exm*dev,samples))
  data_mean=np.average(datos2,axis=1)
  data_std=np.std(datos2,axis=1)
  print data_mean 
  print data_std
#  red=datos2[:exm]
#  mean_datosf=np.average(red,axis=1)
#  print mean_datosf
#  std_datosf=np.std(red,axis=1)
#  
#  red=datos2[samples:samples*2,:]
#  mean_datosg=np.average(red,axis=1)
#  std_datosg=np.std(red,axis=1)
#  
#  red=datos2[samples*2:samples*3,:]
#  mean_datosc=np.average(red,axis=1)
#  std_datosc=np.std(red,axis=1)
#  
  t=[0, 5, 10, 15, 20, 25, 30]
  # t=[3907430*4, 1953715*4, 1302476*4, 976857*4, 781486*4, 651238*4]
  #55820434*4 ]
  #[4096*4, 40960*4, 204800*4, 409600*4, 4096000*4]
   #width= 0.25
  fig, ax = plt.subplots() 
  plt.plot(t,np.roll(data_mean[:exm],1), 'ro-', t,np.roll(data_mean[exm:exm*2],1), 'bo-',t, np.roll(data_mean[exm*2:exm*3],1),'yo-',label={'FPGA', 'GPU','CPU'})
  #plt.xscale('log')
  #text_labels= ('FPGA','GPU','CPU','FGC','FG','GC','FC')
  #colors = plt.cm.Pastel1(np.linspace(0,0.8 , exm))
  ##colors = plt.cm.viridis(np.linspace(0,0.8 , exm))

  #plt.bar(index,mean_datos2,width,color=colors,yerr=std_datos2)
  #plt.xticks(index,text_labels)
  plt.grid(True) 
 
  # Adjust layout to make room for the table:
  plt.subplots_adjust(left=0.2, bottom=0.2)
 
  ax.set_ylabel('Throughtput(Mb/s)')
  ax.set_title(title_name)
  plt.savefig(figure_name) 
  plt.show()




if __name__ == "__main__":
  main() 
