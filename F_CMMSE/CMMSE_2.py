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
import os
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

  os.chdir("./..")
  parser = argparse.ArgumentParser(description='Plot scheduler data.')
  parser.add_argument('fname', help='File prefix for reading the input data')
  parser.add_argument('--dir', help='Directory containing the input data.')
  bar_cycle = (cycler('hatch', ['///', '--', '...','\///', 'xxx', '\\\\']) * cycler('color', 'w')*cycler('zorder', [10]))
  styles = bar_cycle()
  
  title_name=""
  figure_name="CMMSE_Pwd.pdf"
  
  fig, ax = plt.subplots(1,1, figsize=(4,2))
  width= 1
  x=1

#data form CMMSE.py matmult and sobel
  power=np.array([[1,0.108,0.3675,0.0788],[0.977,0.211,1,0.1629]]) 
  print power
  power=power.transpose()
  print power
  x=np.array([1,7])
  print x
  
  colores=['steelblue', 'orange','darkseagreen','darkslategray']
  for i in range (0,4)  :
   h= ax.bar(x+i,power[i,:],color=colores[i]) 
   
  ax.legend(['CPU', 'GPU', 'FPGA', 'Heterogeneous'],bbox_to_anchor=(-0.3,-0.1),loc='upper left', ncol=4)


  text_labels= ('','      Matrix Multiplication   ','  ','','  Sobel Filter')

  ax.set_xticklabels(text_labels)
  plt.grid(True)
  ax.grid(linestyle='--', linewidth=0.3)
  ax.set_ylabel('Normalized time')
  ax.set_title(title_name)
  plt.savefig(figure_name, bbox_inches="tight") 
  
  plt.show()

if __name__ == "__main__":
  main() 
